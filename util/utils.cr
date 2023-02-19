def hex_str(n : UInt8 | UInt16 | UInt32 | UInt64, prefix = false) : String
  (prefix ? "0x" : "") + "#{n.to_s(16).rjust(sizeof(typeof(n)) * 2, '0').upcase}"
end
