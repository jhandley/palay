#!/usr/bin/lua

local palay = require "libpalay"
local doc = palay.newDocument()
doc:paragraph("Hello Palay!")
doc:saveAs("HelloPalay.pdf")

