-- usage: wireshark -X lua_script:.../ncsi-dis.lua foo.cap

local ncsi = Proto("ncsi", "NCSI control protocol")

local types = {
  [0x00] = "Clear Initial State Request",
  [0x80] = "Clear Initial State Response",

  [0x01] = "Select Package Request",
  [0x81] = "Select Package Response",

  [0x02] = "Deselect Package Request",
  [0x82] = "Deselect Package Response",

  [0x03] = "Enable Channel Request",
  [0x83] = "Enable Channel Response",

  [0x04] = "Disable Channel Request",
  [0x84] = "Disable Channel Response",

  [0x05] = "Reset Channel Request",
  [0x85] = "Reset Channel Response",

  [0x06] = "Enable Channel Network Tx Request",
  [0x86] = "Enable Channel Network Tx Response",

  [0x07] = "Disable Channel Network Tx Request",
  [0x87] = "Disable Channel Network Tx Response",

  [0x08] = "AEN Enable Request",
  [0x88] = "AEN Enable Response",

  [0x09] = "Set Link Request",
  [0x89] = "Set Link Response",

  [0x0A] = "Get Link Status Request",
  [0x8A] = "Get Link Status Response",

  [0x0B] = "Set VLAN Filter Request",
  [0x8B] = "Set VLAN Filter Response",

  [0x0C] = "Enable VLAN Request",
  [0x8C] = "Enable VLAN Response",

  [0x0D] = "Disable VLAN Request",
  [0x8D] = "Disable VLAN Response",

  [0x0E] = "Set MAC Address Request",
  [0x8E] = "Set MAC Address Response",

  [0x10] = "Enable Broadcast Filter Request",
  [0x90] = "Enable Broadcast Filter Response",

  [0x11] = "Disable Broadcast Filter Request",
  [0x91] = "Disable Broadcast Filter Response",

  [0x12] = "Enable Global Multicast Filter Request",
  [0x92] = "Enable Global Multicast Filter Response",

  [0x13] = "Disable Global Multicast Filter Request",
  [0x93] = "Disable Global Multicast Filter Response",

  [0x14] = "Set NC-SI Flow Control Request",
  [0x94] = "Set NC-SI Flow Control Response",

  [0x15] = "Get Version ID Request",
  [0x95] = "Get Version ID Response",

  [0x16] = "Get Capabilities Request",
  [0x96] = "Get Capabilities Response",

  [0x17] = "Get Parameters Request",
  [0x97] = "Get Parameters Response",

  [0x18] = "Get Controller Packet Statistics Request",
  [0x98] = "Get Controller Packet Statistics Response",

  [0x19] = "Get NC-SI Statistics Request",
  [0x99] = "Get NC-SI Statistics Response",

  [0x1A] = "Get NC-SI Passthrough Statistics Request",
  [0x9A] = "Get NC-SI Passthrough Statistics Response",

  [0x1B] = "Get Package Status Request",
  [0x9B] = "Get Package Status Response",

  [0x50] = "OEM Request",
  [0xD0] = "OEM Response",

  [0x51] = "PLDM Request",
  [0xD1] = "PLDM Response",

  [0x52] = "Get Package UUID Request",
  [0xD2] = "Get Package UUID Response",
}
setmetatable(types, {
  __index = function(t, k)
    return "UNKNOWN"
  end
})

local resCode = {
  [0x00] = "Command Completed",
  [0x01] = "Command Failed",
  [0x02] = "Command Unavailable",
  [0x03] = "Command Unsupported",
}
setmetatable(resCode, {
  __index = function(t, k)
    return "UNKNOWN"
  end
})

local reaCode = {
  [0x0000] = "No Error",
  [0x0001] = "Interface Initialization Required",
  [0x0002] = "Parameter Invalid",
  [0x0003] = "Channel Not Ready",
  [0x0004] = "Package Not Ready",
  [0x0005] = "Invalid Payload Length",
  [0x7FFF] = "Unsupported Command",
}
setmetatable(reaCode, {
  __index = function(t, k)
    return "UNKNOWN"
  end
})

function ncsi.dissector(buf, pkt, tree)
  pkt.cols.protocol = "NCSI-CTL"
  if buf:len() < 16 then
    return
  end

  local subtree = tree:add(ncsi, buf(), "NCSI Control Protocol Data")

  subtree:add(buf(0,1), "MC ID: " .. buf(0,1))
  subtree:add(buf(1,1), "HdrRev: " .. buf(1,1))
  subtree:add(buf(2,1), "Rsvd: " .. buf(2,1))
  subtree:add(buf(3,1), "IID: " .. buf(3,1))

  local typ = buf(4,1):uint()
  subtree:add(buf(4,1), "Control Packet Type: " .. buf(4,1) .. " (" .. typ .. ") " .. types[typ])
  subtree:add(buf(5,1), "Ch. ID: " .. buf(5,1))
  subtree:add(buf(6,2), "Payload Len: " .. buf(6,2))

  subtree:add(buf(8,4), "Rsvd: " .. buf(8,4))
  subtree:add(buf(12,4), "Rsvd: " .. buf(12,4))

  local info = "[" .. buf(3,1):uint() .. "] " .. types[typ]

  if typ >= 0x80 then
    subtree:add(buf(16,2), "Response Code: " .. buf(16,2) .. " (" .. buf(16,2):uint() .. ") " .. resCode[buf(16,2):uint()])
    subtree:add(buf(18,2), "Reason Code: " .. buf(18,2) .. " (" .. buf(18,2):uint() .. ") " .. reaCode[buf(18,2):uint()])
    info = info .. " - " .. resCode[buf(16,2):uint()] .. ", " .. reaCode[buf(18,2):uint()]
  end

  if typ == 0x8A then
    subtree:add(buf(20,4), "Link Status (up=" .. buf(20,4):bitfield(31-0,1) .. ", speed=" .. buf(20,4):bitfield(31-4,4) .. ", anEn=" .. buf(20,4):bitfield(31-5,1) .. ", anDone=" .. buf(20,4):bitfield(31-6,1) .. ")")
    subtree:add(buf(24,4), "Other Indications")
    subtree:add(buf(28,4), "OEM Link Status")
  end

  pkt.cols.info:set(info)
end

local eth_table = DissectorTable.get("ethertype")
eth_table:add(0x88f8, ncsi)
