local M = {}

function M.OnInit(self, entity)

end

function M.OnUpdate(self, entity, dt)
    entity:SetPosition(100, 0, 0)
end

function M.OnDestroy(self, entity)

end

return M
