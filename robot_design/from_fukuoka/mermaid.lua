local system = require 'pandoc.system'

local function has_mermaid_class(block)
  for _, class in ipairs(block.classes) do
    if class == 'mermaid' then
      return true
    end
  end
  return false
end

function CodeBlock(block)
  if not has_mermaid_class(block) then
    return nil
  end

  local img_data = nil

  system.with_temporary_directory('mermaid', function(tmpdir)
    system.with_working_directory(tmpdir, function()
      local f = io.open('diagram.mmd', 'w')
      f:write(block.text)
      f:close()

      local ok = os.execute('mmdc -i diagram.mmd -o diagram.png -b transparent')

      if ok ~= true and ok ~= 0 then
        error('mmdc failed while rendering a Mermaid diagram')
      end

      local img = io.open('diagram.png', 'rb')
      img_data = img:read('*all')
      img:close()
    end)
  end)

  local filename = pandoc.sha1(block.text) .. '.png'
  pandoc.mediabag.insert(filename, 'image/png', img_data)

  return pandoc.Para({
    pandoc.Image({ pandoc.Str('Mermaid diagram') }, filename)
  })
end
