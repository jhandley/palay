#!/usr/bin/lua

local palay = require "palay"
local doc = palay.newDocument()
doc:paragraph("Hello Palay!")
doc:saveAs("HelloPalay.pdf")

