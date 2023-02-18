#pragma once

#include <game/bvhCollisionSystem.hpp>
#include <engine/camera.hpp>
#include <json/JsonValue.hpp>
#include <game/physicsProfile.hpp>
#include <game/demo.hpp>

#include <filesystem>

// TODO: Replay system. Mouse position would need to be either saved in world space or later transformed by the camera transform. Could store 2 camera transforms one for the actual camera and the replay camera.

// Entites in the editor have to be polymorphic. Can't use inheritance because the pointers might get invalidated
/*
struct Entity {
	EntityType type;
	// Don't need to use verioning in the editor.
	i32 handle;
}

Accessing for example an entity's pos could be done polymorphically at runtime either using arrays which would require all of the arrays to be of the same type or using a switch (could use higher order macros).

The order in which the entites are added in the editor affects the order in which the collision constraints will be resolved at runtime. Can't think of any consistent way to deal with this.

Relational database theory. Seperate tables with 2 foreign keys.

https://box2d.org/documentation/md__d_1__git_hub_box2d_docs_loose_ends.html
*/

/*
Serializing references.

Make a different entity for different kinds of references.
For example circle to circle joint box to box joint and box to circle joint.
This could be handled polymorphically in the editor by making commands that can store multiple commands. Changing the type of entity that a joint used would delete the entity and then create a new one.

Make them store Entity handles like the one above. 
@SerializedOnly
There could be a function that generates all the entity references that a struct stores (std::vector<Entity*> pointer so the can be updated).
When serizaling create a copy of the state. Remove all the entites that are only maintained by commands while creating a mapping from index to index. 
Then using this function update the references of all the entites.
Some entites that don't store polymorphic refences could also have this kind of function that returns the polymorphic entites to make everything the same.
With the array of entites in the editor also store a std::vector<bool> that can be used as the marked flag like in a garbage collector.
The editor state should be maintaned when swithing to game view so things can be undone when switched back.

Should the editor be a part of the game maybe or should it be a seperate class.
They need to store different types of data but they could use the same renderer.

Make data files for colliders. Don't have to use references just serialize recursively.
Maybe make custom editor for selecting the collider type.

Bodies will need to use indices anyway so maybe just use them as entites.
Could later also make custom entites be serialized later.
Could make a BodyEntity that just has to texture assigned to it and a body.

In the editor null references have to be allowed for example for joints.

Joints should be stored in the body so when a body is removed the joint is too.
struct JointId {
	i32 index;
	JointType type;
}
*/

// TODO: https://youtu.be/xvAVQ6GEv-E?t=298
class Game {
public:
	Game();

	auto saveLevel() const -> Json::Value;
	auto saveLevelToFile(std::string_view path) -> void;
	[[nodiscard]] auto loadLevel(const Json::Value& level) -> bool;
	auto loadLevelFromFile(const char* path) -> std::optional<const char*>;
	auto loadDemo(Demo& demo) -> void;
	auto drawUi() -> void;
	auto openLoadLevelDialog() -> std::optional<const char*>;
	auto openSaveLevelDialog() -> void;
	// This is split into 2 function like this so the popup with the name error is only opened in one place of the code which means there can only be one at a time. This allows calling openErrorPopupModal from anywhere.
	const char* errorPopupModalMessage = "";
	bool openErrorPopup = false;
	auto openErrorPopupModal(const char* message) -> void;
	auto displayErrorPopupModal() -> void;
	auto update() -> void;
	auto physicsStep(float dt, i32 solverIterations, PhysicsProfile& profile) -> void;
	auto draw(Vec2 cursorPos) -> void;
	auto resetLevel() -> void;

	std::optional<std::string> lastSavedLevelPath;

	std::vector<Json::Value> levelSavestates;

	// Could create a system similar to the demo system, but for tools, but some tools require getting some specific data so I am not sure if it is worth doing. Could make a struct that just contains all the data needed for tools. struct ToolData.
	// The update method would need to be called even if the tool isn't selected to tools can create shortcuts and thing like that.
	// There would also need to be a way to register the Buttons in Input
	enum class Tool {
		GRAB,
		SELECT,
		DISTANCE_JOINT,
		REVOLUTE_JOINT,
		// Maybe make something that lets you click once to choose circle center and the another time to choose a point on the circle which so the distance between the points is the radius.
		CREATE_BODY,
		CREATE_LINE,
		TRAIL,
		DISABLE_COLLISON,
		// TODO: Graphing tool. 
		// if nothing selected display "no entity selected"
		// when selected make a combo with the possible graphs
		// pos.x, pos.y, vel, rotation
		// Save to level.
		// Phase space Graphs?
	};
	Tool selectedTool = Tool::GRAB;
	bool selectingJointTool = false;

	std::optional<BodyId> grabbed;
	Vec2 grabPointInGrabbedObjectSpace;

	auto selectToolGui() -> void;
	auto selectToolUpdate(Vec2 cursorPos, const std::optional<BodyId>& bodyUnderCursor) -> void;
	auto selectToolDraw() -> void;

	using Entity = std::variant<BodyId, DistanceJointId, TrailId>;
	std::optional<Entity> selected;
	bool focusingOnSelected = false;
	float elapsedSinceFocusStart = 0.0f;
	Vec2 lastFrameFocusPos;

	std::optional<BodyId> distanceJointBodyA;
	Vec2 distanceJointBodyAAnchor;

	enum class BodyShape {
		CIRCLE, RECTANGLE
	};
	BodyShape selectedShape = BodyShape::CIRCLE;
	float circleRadius = 1.0f;
	Vec2 boxSize{ 1.0f };
	float boxOrientation = 0.0f;

	float lineWidth = 1.0f;
	bool isLineStatic = false;
	bool endpointsInside = true;
	bool chainLine = false;
	std::optional<Vec2> lineStart;

	CollisionMap contacts;

	std::vector<std::unique_ptr<Demo>> demos;
	std::optional<Demo&> loadedDemo;

	std::optional<BodyId> disableCollisionBodyA;

	Camera camera;

	Vec2 gravity{ 0.0f };
	float angularDamping = 0.98f;

	std::optional<Vec2> grabStart;

	bool updatePhysics = true;
	bool drawContacts = false;
	bool scaleContactNormals = false;
	bool doASingleStep = false;

	PhysicsProfile physicsProfile;
	int profileUnitsIndex = 0;

	int physicsSolverIterations = 10;
	int physicsSubsteps = 1;

	bool drawTrajectory = false;
	Vec2 initialVelocity{ 1.0f };

	bool isGridEnabled = true;
	bool automaticallyScaleGrid = true;
	float gridCellSize = 1.0f;

	bool snapToGrid = true;
	bool snapToObjects = true;


	// This is also called warm starting.
	// The physics engine uses an iterative systems of equation solver which uses the Gauss-Seidel method. It starts with an initial guess and tries to get as close as possible to the analytical solution (if one exists else it gets it closer to satifying all equations). Enabling this makes it so the solver tries to improve convergence by expoliting temporal coherence between solutions. It uses the solution from the previous frame as the starting guess for the new frame.
	static bool usePreviousStepImpulseSolutionsAsInitialGuess;
	static bool positionCorrection;
	static bool accumulateImpulses;

	BvhCollisionSystem collisionSystem;
};