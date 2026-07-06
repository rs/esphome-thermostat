|-
  const std::string hvac_modes = id(ha_hvac_modes).state;
  auto token_contains = [](const std::string &text, const std::string &needle) {
    size_t pos = 0;
    while (pos < text.size()) {
      while (pos < text.size()) {
        const char c = text[pos];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
          break;
        }
        pos++;
      }
      const size_t start = pos;
      while (pos < text.size()) {
        const char c = text[pos];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
          break;
        }
        pos++;
      }
      if (pos > start && text.substr(start, pos - start) == needle) return true;
    }
    return false;
  };
  return token_contains(hvac_modes, "auto") && !token_contains(hvac_modes, "heat_cool");
