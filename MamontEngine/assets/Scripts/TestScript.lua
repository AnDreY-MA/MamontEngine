local owner = {}
local audioComp = {}

local currentTransform = {}
local i = 21

local newPosition = Vector3.new(4.0, 10.0, -3.0)

function BeginPlay()
	owner = ScriptComponent:GetOwner()
	local name = owner:GetTagComponent()
	Log.Info(name.name)

	currentTransform = owner:GetTransformComponent()
end

function Update(dt)
	currentTransform:SetPosition(newPosition)

	if Input.IsKeyPressed(Key.A) then
		Log.Info("A pressed")
	end
end
