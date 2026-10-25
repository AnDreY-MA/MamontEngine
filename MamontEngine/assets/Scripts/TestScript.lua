local owner = {}
local audioComp = {}

local count = {}

function BeginPlay()
	owner = ScriptComponent:GetOwner()
	local name = owner:GetTagComponent()
	Log.Info(name.name)

	audioComp = owner:GetAudioComponent()
	count = 1 
end

function Update(dt)
	if count == 1 then
		audioComp:Play()
		count = 0
	end
	newPosition = Vector3.new(4.0, 10.0, -3.0)
	newPosition.x = 2.0
	owner:GetTransformComponent():SetPosition(newPosition) 
	if Input.IsKeyPressed(Key.A) then
		Log.Info("Space pressed")
	end
end
