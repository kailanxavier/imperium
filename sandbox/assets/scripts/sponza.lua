---@type Script
local M = {}

local lastPos

function M.OnInit(self, entity)

end

function M.OnUpdate(self, entity, dt)
    local currentPos = entity:GetPosition()
    if currentPos ~= lastPos then
        lastPos = currentPos
        print("Current pos:", currentPos)
    end
end

function M.OnDestroy(self, entity)
    
end

return M
