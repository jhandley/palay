-- Copyright 2014 LKC Technologies, Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

function header(content)
    local leftMargin, topMargin, rightMargin, bottomMargin = getPageMargins()
    for i = 1, getPageCount() do
        -- Put the header in the top margin i.e. between top of page
        -- and topMargin.
        pushStyle({width = getPageWidth() - leftMargin - rightMargin, height = topMargin})
        startBlock("BottomLeft", leftMargin, (i - 1) * getPageHeight() + topMargin)
        if type(content) == "function" then
          text(content(i, getPageCount()))
        else
          text(content)
        end
        endBlock()
        popStyle()
    end
end

function footer(content)
    local leftMargin, topMargin, rightMargin, bottomMargin = getPageMargins()
    for i = 1, getPageCount() do
        -- Put the footer in the bottom margin i.e. between bottom margin
        -- and bottom of page.
        pushStyle({width = getPageWidth() - leftMargin - rightMargin, height = bottomMargin})
        startBlock("TopLeft", leftMargin, i * getPageHeight() - bottomMargin)
        if type(content) == "function" then
          text(content(i, getPageCount()))
        else
          text(content)
        end
        endBlock()
        popStyle()
    end
end
