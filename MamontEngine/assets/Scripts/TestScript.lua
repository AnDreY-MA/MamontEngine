
local owner = {}

function BeginPlay()
	--local name = ScriptComponent:GetOwner():GetTagComponent()
	--Log.Info(name.name)
end

function Update(dt)
	owner = ScriptComponent:GetOwner()
	local name = owner:GetTagComponent()
	--Log.Info(name.name)
	newPosition = Vector3.new(4.0, 10.0, -3.0)
	newPosition.x = 2.0
	owner:GetTransformComponent():SetPosition(newPosition)
	if Input.IsKeyPressed(Key.A) then
		Log.Info("Space pressed")
	end
	--Log.Info(transform.x)
end
