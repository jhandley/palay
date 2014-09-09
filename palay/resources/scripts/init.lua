local palay = require "libpalay"

local palaydocument = palay.newDocument()

-- For each method in palaydocument create
-- a global function that calls the method on
-- the global document.
style = function(...) return palaydocument:style(...) end
text = function(...) return palaydocument:text(...) end
paragraph = function(...) return palaydocument:paragraph(...) end
saveAs = function(...) return palaydocument:saveAs(...) end

Normal = palay.Normal
Bold = palay.Bold
Italic = palay.Italic
Underline = palay.Underline
