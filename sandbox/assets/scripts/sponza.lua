---@type Script
local M = {}

local lastPos

function M.OnInit(self, entity)
    Log.Info("Hello I am an info message!")
    Log.Warning("Hello I am a warning message!")
    Log.Error("Hello I am an error message!")
    Log.Fatal("Hello I am a fatal message!")
end

function M.OnUpdate(self, entity, dt)
    local currentPos = entity:GetPosition()
    if currentPos ~= lastPos then
        lastPos = currentPos
        print("Current pos:", currentPos)
    end
end

function M.OnDestroy(self, entity)
    Log.Info("Good byte.")
end

return M
