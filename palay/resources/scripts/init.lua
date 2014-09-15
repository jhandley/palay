
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
