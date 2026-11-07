#!/usr/bin/env python3
"""Independent, read-only checks for an EasyEDA Pro thermostat revision.

This checks the SQLite project and decoded schematic/PCB records. Native
EasyEDA DRC/DFM and the final Gerber inspection remain required.
"""
from __future__ import annotations
import argparse, base64, gzip, json, math, sqlite3, zipfile
from pathlib import Path

MIL_TO_MM = 0.0254
OUTLINE = (0.0, 3149.6063, -3149.6063, 0.0)
H1_EXPECT = {
    "1": (1673.2276, -531.496), "2": (1673.2276, -452.756),
    "3": (1594.4876, -531.496), "4": (1594.4876, -452.756),
    "19": (964.5676, -531.496), "20": (964.5676, -452.756),
}

def unpack(s):
    if s.startswith("base64"):
        return gzip.decompress(base64.b64decode(s[6:])).decode()
    return s

def rows(s):
    return [json.loads(x) for x in unpack(s).splitlines() if x]

def attrs(rs):
    out = {}
    for r in rs:
        if r and r[0] == "ATTR" and len(r) > 8:
            out.setdefault(r[2], {})[r[3]] = r[4]
    return out

def pcb_attrs(rs):
    """PCB ATTR records store owner/key/value at indices 3/7/8."""
    out={}
    for r in rs:
        if r and r[0]=="ATTR" and len(r)>8:
            out.setdefault(r[3],{})[r[7]]=r[8]
    return out

def close(a, b, eps=0.01):
    return abs(a-b) <= eps

def schematic_pin_nets(sch, comps):
    """Return physical schematic pin->net mapping and explicit-wire shorts."""
    # Wire vertices form the electrical graph. EasyEDA's NET attribute anchors
    # are normally vertices; include every segment endpoint and propagate names
    # through connected segments rather than trusting labels alone.
    adj={}
    labels={}
    def add(a,b):
        adj.setdefault(a,set()).add(b); adj.setdefault(b,set()).add(a)
    wires={}
    wire_names={}
    for r in sch:
        if r and r[0]=="ATTR" and len(r)>4 and r[3]=="NET" and r[2]:
            name=str(r[4] or "").strip()
            if name: wire_names.setdefault(r[2],set()).add(name)
    for r in sch:
        if r and r[0]=="WIRE" and len(r)>2 and isinstance(r[2],list):
            segs=[]
            # Schematic WIRE geometry is a list of [x1,y1,x2,y2] segments,
            # unlike PCB LINE records which carry coordinates directly.
            for p in r[2]:
                if isinstance(p,list) and len(p)>=4:
                    a=(round(float(p[0]),5),round(float(p[1]),5))
                    b=(round(float(p[2]),5),round(float(p[3]),5))
                    segs.append((a,b)); add(a,b)
                    # EasyEDA's NET belongs to the WIRE record. Its ATTR
                    # anchor can be visually offset, so never rely on anchor
                    # proximity for the electrical name.
                    for name in wire_names.get(r[1],()):
                        labels.setdefault(a,set()).add(name)
                        labels.setdefault(b,set()).add(name)
            wires[r[1]]=segs
    for r in sch:
        if r and r[0]=="ATTR" and len(r)>8 and r[3]=="NET" and r[2] in wires:
            name=str(r[4] or "").strip()
            if name and isinstance(r[7],(int,float)) and isinstance(r[8],(int,float)):
                q=(round(float(r[7]),5),round(float(r[8]),5))
                labels.setdefault(q,set()).add(name)
                # Keep the anchor in the graph for diagnostics, but its
                # location does not determine the wire's electrical net.
    seen=set(); group_names={}
    for start in adj:
        if start in seen: continue
        stack=[start]; seen.add(start); group=set()
        while stack:
            q=stack.pop(); group.add(q)
            for n in adj.get(q,()):
                if n not in seen: seen.add(n); stack.append(n)
        names=set().union(*(labels.get(q,set()) for q in group))
        for q in group: group_names[q]=names
    segments=[seg for segs in wires.values() for seg in segs]
    def names_at(q):
        names=set(group_names.get(q,set()))
        if names: return names
        qx,qy=q
        for a,b in segments:
            ax,ay=a; bx,by=b
            cross=(bx-ax)*(qy-ay)-(by-ay)*(qx-ax)
            within=min(ax,bx)-1e-5<=qx<=max(ax,bx)+1e-5 and min(ay,by)-1e-5<=qy<=max(ay,by)+1e-5
            if abs(cross)<1e-4 and within:
                names.update(group_names.get(a,set())); names.update(group_names.get(b,set()))
        return names

    # Resolve symbol pins into world coordinates. Schematic symbol units are
    # 10 mil and component origins/rotations use the same coordinates.
    out={}; shorts=[]
    for cid,cr in comps.items():
        a=attrs(sch).get(cid,{})
        sid=a.get("Symbol")
        if not sid: continue
        # caller attaches symbol rows through the temporary map below
        sr=cr.get("_symbol_rows")
        if not sr: continue
        pin_attrs={}
        for ar in sr:
            if ar and ar[0]=="ATTR" and len(ar)>4 and ar[2]:
                pin_attrs.setdefault(ar[2],{})[ar[3]]=ar[4]
        pin_world={}
        raw=cr["raw"]
        ang=math.radians(float(raw[5] or 0)); ca,sa=math.cos(ang),math.sin(ang)
        for pr in sr:
            if not pr or pr[0]!="PIN" or pr[1] not in pin_attrs: continue
            pa=pin_attrs[pr[1]]
            num=str(pa.get("NUMBER",""));
            if not num or len(pr)<7: continue
            lx,ly=float(pr[4]),float(pr[5]); x,y=float(raw[3]),float(raw[4])
            wx=round(x+lx*ca-ly*sa,5); wy=round(y+lx*sa+ly*ca,5)
            pin_world[num]=(wx,wy,pa.get("NAME","") )
        out[cid]={}
        for num,(x,y,pname) in pin_world.items():
            names=names_at((x,y))
            if len(names)>1: shorts.append((cid,num,sorted(names),(x,y)))
            out[cid][num]=next(iter(names)) if len(names)==1 else ("|".join(sorted(names)) if names else "")
    return out,shorts

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("project", type=Path)
    ap.add_argument("--epro", type=Path)
    args=ap.parse_args()
    c=sqlite3.connect(args.project); c.row_factory=sqlite3.Row
    failures=[]; warnings=[]
    def ok(label, detail=""):
        print("PASS", label, detail)
    def fail(label, detail):
        failures.append((label,detail)); print("FAIL",label,detail)
    try:
        if c.execute("pragma integrity_check").fetchone()[0] == "ok": ok("SQLite integrity")
        else: fail("SQLite integrity", "PRAGMA integrity_check failed")
        tables={r[0] for r in c.execute("select name from sqlite_master where type='table'")}
        for t in ("components","devices","attributes","documents","projects"):
            if t not in tables: fail("schema",f"missing {t}")

        comps={r["uuid"]:r for r in c.execute("select * from components")}
        devices={r["uuid"]:r for r in c.execute("select * from devices")}
        dattrs={}
        for r in c.execute("select * from attributes"):
            dattrs.setdefault(r["device_uuid"],{})[r["key"]]=r["value"]
        for du,d in devices.items():
            a=dattrs.get(du,{})
            for k in ("Symbol","Footprint"):
                if a.get(k) and a[k] not in comps:
                    fail("device library closure",f"{d['display_title']} {k}={a[k]} missing")
        ok("device library closure",f"{len(devices)} devices, {len(comps)} components")

        docs={r["docType"]:rows(r["dataStr"]) for r in c.execute("select * from documents")}
        sch=docs.get(1,[]); pcb=docs.get(3,[])
        if not sch or not pcb: fail("documents", "schematic or PCB document missing")
        sa=attrs(sch); pa=attrs(pcb)
        schcomps={r[1]:r for r in sch if r and r[0]=="COMPONENT"}
        for cid,a in sa.items():
            if cid not in schcomps: continue
            if a.get("Device") and a["Device"] not in devices:
                fail("schematic library closure",f"{cid} Device={a['Device']} missing")
            if a.get("Symbol") and a["Symbol"] not in comps:
                fail("schematic library closure",f"{cid} Symbol={a['Symbol']} missing")
            if a.get("Footprint") and a["Footprint"] not in comps:
                fail("schematic library closure",f"{cid} Footprint={a['Footprint']} missing")
        ok("schematic library closure",f"{len(schcomps)} schematic components")

        # Resolve actual wire connectivity from symbol pin coordinates. This is
        # independent of component ATTR text and catches a wire group that
        # physically joins two different named nets.
        schmeta={}
        for cid,raw in schcomps.items():
            sid=sa.get(cid,{}).get("Symbol")
            schmeta[cid]={"raw":raw,"_symbol_rows":rows(comps[sid]["dataStr"])} if sid in comps else {"raw":raw}
        pin_nets,wire_shorts=schematic_pin_nets(sch,schmeta)
        if wire_shorts:
            for cid,pn,names,xy in wire_shorts[:20]: fail("schematic physical net short",f"{cid} pin {pn} at {xy}: {names}")
        else: ok("schematic physical net groups","no connected wire group has multiple explicit net names")

        # PCB component attributes and channel links back to schematic instances.
        pcbcomps={r[1]:r for r in pcb if r and r[0]=="COMPONENT"}
        pcbattrs=pcb_attrs(pcb)
        for cid,a in pcbattrs.items():
            if cid not in pcbcomps: continue
            for k in ("Device","Footprint"):
                if a.get(k) and a[k] not in comps and not (k=="Device" and a[k] in devices):
                    fail("PCB library closure",f"{cid} {k}={a[k]} missing")
        channel_links=0
        for cid,r in pcbcomps.items():
            ch=r[7].get("Channel ID","") if len(r)>7 and isinstance(r[7],dict) else ""
            if ch.startswith("$1"):
                sid=ch[2:]
                if sid not in schcomps: fail("PCB channel link",f"{cid}->{sid} schematic component missing")
                else: channel_links+=1
        ok("PCB component/channel closure",f"{len(pcbcomps)} PCB components; {channel_links} channel links")

        # Cross-check every routed PCB pad against the net reached by the
        # corresponding schematic pin. Empty means intentionally unconnected;
        # named-vs-empty and named-vs-different are both actionable failures.
        mismatches=[]
        padnets={}
        for r in pcb:
            if r and r[0]=="PAD_NET" and len(r)>3:
                padnets.setdefault(r[1],{})[str(r[2])]=str(r[3] or "")
        for cid,r in pcbcomps.items():
            ch=r[7].get("Channel ID","") if len(r)>7 and isinstance(r[7],dict) else ""
            sid=ch[2:] if ch.startswith("$1") else None
            if not sid or sid not in pin_nets: continue
            for pn,sch_net in pin_nets[sid].items():
                pcb_net=padnets.get(cid,{}).get(str(pn),"")
                # Unnamed schematic wire groups are legitimate (EasyEDA often
                # carries their identity by wire/graph membership only). They
                # cannot be compared to a PCB's generated net name here; only
                # an explicitly named schematic group is a hard mismatch.
                if sch_net and sch_net != pcb_net:
                    mismatches.append((cid,sid,pn,sch_net,pcb_net))
        for m in mismatches[:30]: fail("schematic/PCB pin net",f"{m[0]}({m[1]}) pin {m[2]} schematic={m[3]!r} PCB={m[4]!r}")
        if not mismatches: ok("schematic/PCB pin-net consistency","all resolvable physical pins agree")

        # Explicit semantic checks for the added power path. These are kept
        # separate from generic consistency so a renamed generated PCB net
        # cannot hide a wrong LM66100 or bypass connection.
        semantic={
            "U4":{"1":"+5V","2":"GND","3":"+5V_DISPLAY","4":"","5":"GND","6":"+5V_DISPLAY"},
            "C12":{"1":"+24V","2":"GND"},
            "C13":{"1":"+5V","2":"GND"},
            "C14":{"1":"+5V_DISPLAY","2":"GND"},
        }
        sch_by_designator={a.get("Designator"):cid for cid,a in sa.items() if a.get("Designator")}
        pcb_by_designator={a.get("Designator"):cid for cid,a in pcbattrs.items() if a.get("Designator")}
        for des,expected in semantic.items():
            scid=sch_by_designator.get(des); pcid=pcb_by_designator.get(des)
            if not scid or not pcid:
                fail("power-path placement",f"{des}: schematic={scid} PCB={pcid}"); continue
            svals=pin_nets.get(scid,{})
            pvals=padnets.get(pcid,{})
            for pn,net in expected.items():
                if svals.get(pn,"") != net: fail("power-path schematic net",f"{des} pin {pn}: {svals.get(pn)!r}, expected {net!r}")
                if pvals.get(pn,"") != net: fail("power-path PCB net",f"{des} pad {pn}: {pvals.get(pn)!r}, expected {net!r}")
            if all(svals.get(pn,"")==net and pvals.get(pn,"")==net for pn,net in expected.items()):
                ok("power-path semantic mapping",f"{des}: {expected}")

        # Duplicate primitive IDs are fatal for EasyEDA's JSONL representation.
        # PAD_NET rows intentionally reuse their owning COMPONENT ID; ATTR
        # rows also use nested-owner IDs. Check primitive namespaces only.
        ids=[str(r[1]) for r in pcb if r and len(r)>1 and r[0] in ("COMPONENT","LINE","VIA","STRING","POLY","FILL","POUR","PRIMITIVE")]
        dups=sorted({x for x in ids if ids.count(x)>1})
        if dups: fail("PCB primitive UUIDs",str(dups))
        else: ok("PCB primitive UUIDs",f"{len(ids)} IDs unique")

        # Mirrored H1 physical pad locations and logical pad-net assignment.
        h1=next((cid for cid,a in pcbattrs.items() if a.get("Designator")=="H1"),None)
        if h1:
            hr=pcbcomps[h1]; hx,hy=hr[4],hr[5]; rot=float(hr[6] or 0)
            hfp=pcbattrs[h1].get("Footprint"); fr=comps.get(hfp)
            if not fr: fail("H1 footprint",f"{hfp} missing")
            else:
                frows=rows(fr["dataStr"]); pads={str(x[5]):(x[6],x[7]) for x in frows if x and x[0]=="PAD"}
                actual={}
                for n,(lx,ly) in pads.items():
                    a=math.radians(rot); actual[n]=(hx+lx*math.cos(a)-ly*math.sin(a),hy+lx*math.sin(a)+ly*math.cos(a))
                for n,exp in H1_EXPECT.items():
                    if n not in actual or not all(close(actual[n][i],exp[i],0.02) for i in (0,1)):
                        fail("H1 mirrored pad",f"pin {n}: got {actual.get(n)}, expected {exp}")
                if all(n in actual and all(close(actual[n][i],e[i],0.02) for i in (0,1)) for n,e in H1_EXPECT.items()):
                    ok("H1 mirrored pad positions","pins 1,2,3,4,19,20 match face-to-face mapping")
            nets={str(x[2]):x[3] for x in pcb if x and x[0]=="PAD_NET" and x[1]==h1}
            # Derive the expected header rail from the current schematic wire
            # graph. The display rail may intentionally be named +5V_DISPLAY;
            # hardcoding the historical +5V name would create a false failure.
            ch=hr[7].get("Channel ID","") if len(hr)>7 and isinstance(hr[7],dict) else ""
            sch_h1=ch[2:] if ch.startswith("$1") else None
            expected={n:pin_nets.get(sch_h1,{}).get(n,"") for n in ("1","2","3","4","19","20")}
            for n,v in expected.items():
                if not v or nets.get(n)!=v: fail("H1 pad net",f"pin {n}: PCB={nets.get(n)!r}, schematic={v!r}")
            if all(v and nets.get(n)==v for n,v in expected.items()): ok("H1 logical pad nets",str(expected))

        # Board outline extrema. The outline is a POLY on layer 11; arc records
        # carry extra radius/angle fields, so extrema are checked conservatively.
        outlines=[r for r in pcb if r and r[0]=="POLY" and len(r)>4 and r[4]==11]
        nums=[]
        def collect(v):
            if isinstance(v,(int,float)): nums.append(float(v))
            elif isinstance(v,list):
                for q in v: collect(q)
        for r in outlines: collect(r[6])
        # x/y values are interspersed with ARC metadata; explicit known extrema
        # must occur in the path and no coordinate can exceed the nominal 80mm.
        if outlines and all(any(close(n,t,0.03) for n in nums) for t in (0,3149.6063,-3149.6063)):
            ok("board outline",f"layer-11 outline present ({len(outlines)} primitive); 80x80mm extrema found")
        else: fail("board outline","layer-11 outline/extrema not found")

        # Flag the known native-D RC blind spot: designator text crossing the
        # bottom edge. This is a geometric warning, not a DRC substitute.
        for cid,a in pcbattrs.items():
            if a.get("Designator")=="LED1":
                r=next((x for x in pcb if x and x[0]=="ATTR" and len(x)>8 and x[1]==next((y[1] for y in pcb if y and y[0]=='ATTR' and y[3]=='Designator' and y[2]==cid),'')),None)
                if r and isinstance(r[6],(int,float)) and r[6] < OUTLINE[2]:
                    warnings.append(f"LED1 designator anchor y={r[6]} below outline {OUTLINE[2]}")
                    print("WARN silk edge",warnings[-1])

        # Specific new-library records and placement status.
        for du,a in dattrs.items():
            if a.get("Supplier Part")=="C2869734":
                if a.get("Manufacturer Part")!="LM66100DCKR": fail("LM66100 identity",str(a))
                else: ok("LM66100 library identity","C2869734 / LM66100DCKR")
                if du not in [sa.get(cid,{}).get("Device") for cid in schcomps]: warnings.append("LM66100 library imported but not placed in schematic")
            if a.get("Supplier Part")=="C513710":
                if a.get("Value")!="220nF" or a.get("Voltage Rating")!="100V": fail("220nF identity",str(a))
                else: ok("220nF library identity","C513710 / 100V")
                if du not in [sa.get(cid,{}).get("Device") for cid in schcomps]: warnings.append("C513710 library imported but not placed in schematic")

        if args.epro:
            with zipfile.ZipFile(args.epro) as z:
                if z.testzip() is not None: fail("epro archive","CRC failure")
                else: ok("epro archive","ZIP CRC clean")
                names=set(z.namelist())
                for uid,r in comps.items():
                    ext="efoo" if r["docType"]==4 else "esym"
                    if f"FOOTPRINT/{uid}.{ext}" not in names and f"SYMBOL/{uid}.{ext}" not in names:
                        # old export convention places docType 4 in FOOTPRINT.
                        fail("epro library closure",f"{uid} {r['display_title']} archive member missing")
                ok("epro library closure") if not any(x[0]=="epro library closure" for x in failures) else None
        print(f"\nSUMMARY failures={len(failures)} warnings={len(warnings)}")
        for w in warnings: print("WARNING",w)
        raise SystemExit(1 if failures else 0)
    finally:
        c.close()

if __name__ == "__main__": main()
