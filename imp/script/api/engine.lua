---@class Entity
local Entity = {}

--- Gets the entity's local transform position.
---
--- Returns nil if the entity has been destroyed or does not
--- have a transform component
---
--- @return vec3f? position The entity's local position, or nil.
function Entity:GetPosition() end

--- Sets the entity's local transform position.
---
--- @param x number X position.
--- @param y number Y position.
--- @param z number Z position.
function Entity:SetPosition(x, y, z) end

--- Gets the entity's local transform rotation.
---
--- Returns nil if the entity has been destroyed or does not
--- have a transform component.
---
--- @return quatf? rotation The entity's rotation, or nil.
function Entity:GetRotation() end

--- Sets the entity's local transform rotation.
---
--- The supplied angles are in degrees and are converted to
--- radians internally.
---
--- @param x number Rotation around the X axis, in degrees.
--- @param y number Rotation around the Y axis, in degrees.
--- @param z number Rotation around the Z axis, in degrees.
function Entity:SetRotation(x, y, z) end

--- Controls whether the entity's renderable component is marked
--- as visible.
---
--- Does nothing if the entity is no longer alive or does not
--- have a renderable component.
---
--- @param visible boolean Whether the renderable should be visible.
function Entity:SetRenderableVisible(visible) end

---@class vec3f
---@field x number
---@field y number
---@field z number
local vec3f = {}

--- Creates a zero-initialised vector.
--- @return vec3f
function vec3f.new() end

--- Creates a vector from its three components.
--- @param x number
--- @param y number
--- @param z number
--- @return vec3f
function vec3f.new(x, y, z) end

---@class quatf
---@field x number
---@field y number
---@field z number
---@field w number
local quatf = {}

--- Creates an identity quaternion.
--- @return quatf
function quatf.new() end

--- Creates a quaternion from its four components.
--- @param x number
--- @param y number
--- @param z number
--- @param w number
--- @return quatf
function quatf.new(x, y, z, w) end

---@class Transform
---@field position vec3f
---@field rotation quatf
---@field scale vec3f
local Transform = {}

---@class Script
---@field OnInit OnInit
---@field OnUpdate OnUpdate
---@field OnDestroy OnDestroy
local Script = {}

---@param self Script
---@param entity Entity
function Script.OnInit(self, entity) end

---@param self Script
---@param entity Entity
---@param dt number
function Script.OnUpdate(self, entity, dt) end

---@param self Script
---@param entity Entity
function Script.OnDestroy(self, entity) end

---@alias OnInit fun(self: Script, entity: Entity)
---@alias OnUpdate fun(self: Script, entity: Entity, dt: number)
---@alias OnDestroy fun(self: Script, entity: Entity)
