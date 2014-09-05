local palay = require "libpalay"

local palaydocument = palay.newDocument()

-- For each method in palaydocument create
-- a global function that calls the method on
-- the global document.
for k,v in pairs(getmetatable(palaydocument)) do
  if type(v) == "function" then
    _G[k] = function(...) return v(palaydocument, ...) end
  end
end
