#pragma once

#include <game/renderer.hpp>
#include <game/bvhCollisionSystem.hpp>

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

class Game {
public:
	Game();

	auto detectCollisions() -> void;
	auto loadLevel() -> void;
	auto drawUi() -> void;
	auto update(Gfx& gfx, Renderer& renderer) -> void;

	Vec2* controlledValue = nullptr;
	Vec2* followedPos = nullptr;

	Camera camera;

	Vec2 gravity{ 0.0f };
	float angularDamping = 0.98f;

	bool updatePhysics = true;
	bool cameraFollow = true;
	bool drawContacts = false;

	bool drawTrajectory = false;
	Vec2 initialVelocity{ 1.0f };

	static bool warmStarting;
	static bool positionCorrection;
	static bool accumulateImpulses;

	BvhCollisionSystem collisionSystem;
};