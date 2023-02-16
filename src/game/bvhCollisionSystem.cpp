#include <game/bvhCollisionSystem.hpp>
#include <utils/io.hpp>
#include <engine/debug.hpp>

BvhCollisionSystem::BvhCollisionSystem()
	: rootNode{ NULL_NODE }
{}

auto BvhCollisionSystem::update() -> void {
	nodesToRemove.clear();
	for (const auto nodeIndex : leafNodes) {
		if (!ent.body.isAlive(node(nodeIndex).body)) {
			nodesToRemove.push_back(nodeIndex);
		}
	}

	for (const auto& node : nodesToRemove) {
		removeLeafNode(node);
		freeNode(node);
		std::erase(leafNodes, node);
	}

	for (const auto bodyId : ent.body.entitiesAddedLastFrame()) {
		const auto& body = ent.body.get(bodyId);
		if (!body.has_value()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		if (std::find_if(leafNodes.begin(), leafNodes.end(), [&](u32 n) { return node(n).body == bodyId; }) != leafNodes.end()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		insert(bodyId);
	}

}

auto BvhCollisionSystem::reset() -> void {
	leafNodes.clear();
	freeNodes.clear();
	for (usize i = 0; i < nodes.size(); i++) {
		freeNodes.push_back(static_cast<i32>(i));
	}
	rootNode = NULL_NODE;
}

auto BvhCollisionSystem::updateBvh() -> void {
	for (const auto& nodeIndex : leafNodes) {
		auto& node = BvhCollisionSystem::node(nodeIndex);

		const auto& body = ent.body.get(node.body);
		if (!body.has_value()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		// @Performance Add sleeping flag for bodies that haven't moved and check it here.
		if (body->isStatic())
			continue;

		const auto updatedAabb = aabb(body->collider, body->transform);
		if (!(node.aabb.contains(updatedAabb.min) && node.aabb.contains(updatedAabb.max))) {
			if (leafNodes.size() == 1) {
				node.aabb = addMarginToAabb(updatedAabb);
			} else {
				removeLeafNode(nodeIndex);
				node.aabb = addMarginToAabb(updatedAabb);
				insertHelper(rootNode, nodeIndex);
			}

		}
	}
}

#include <chrono>

// @Performance: Don't check collision between sleeping objects.
auto BvhCollisionSystem::detectCollisions(CollisionMap& collisions, const IgnoredCollisions& collisionsToIgnore) -> void {
	if (rootNode == NULL_NODE || node(rootNode).isLeaf())
		return;

	auto& root = node(rootNode);
	clearCrossedFlag(rootNode);
	newCollisions.clear();
	collide(newCollisions, collisionsToIgnore, root.children[0], root.children[1]);

	// Removing inside collide would require in total checking all the variations.
	for (auto it = collisions.begin(); it != collisions.end(); ) {
		const auto& oldCollisionKey = it->first;
		if (newCollisions.find(oldCollisionKey) == newCollisions.end()) {
			it = collisions.erase(it);
		} else {
			++it;
		}
	}

	for (auto& [newCollisionKey, newCollision] : newCollisions) {
		// @Performance: There might be a better function for doing this.
		auto oldCollision = collisions.find(newCollisionKey);
		if (oldCollision == collisions.end()) {
			collisions[newCollisionKey] = newCollision;
		} else {
			oldCollision->second.update(newCollision);
		}
	}
}

auto BvhCollisionSystem::raycast(Vec2 start, Vec2 end) const -> std::optional<RaycastResult> {
	if (rootNode == NULL_NODE)
		return std::nullopt;

	return raycastHelper(rootNode, start, end);
}

auto BvhCollisionSystem::raycastHelper(u32 nodeIndex, Vec2 start, Vec2 end) const -> std::optional<RaycastResult> {
	const auto& node = BvhCollisionSystem::node(nodeIndex);

	if (!node.aabb.rayHits(start, end))
		return std::nullopt;

	if (node.isLeaf()) {
		const auto& body = ent.body.get(node.body);
		if (!body.has_value()) {
			ASSERT_NOT_REACHED();
			return std::nullopt;
		}
		return ::raycast(start, end, body->collider, body->transform);
	}

	const auto result0 = raycastHelper(node.children[0], start, end);
	if (!result0.has_value())
		return raycastHelper(node.children[1], start, end);
	
	const auto result1 = raycastHelper(node.children[1], start, end);
	if (!result1.has_value())
		return result0;

	return result0->t < result1->t ? result0 : result1;
}

auto BvhCollisionSystem::clearCrossedFlag(u32 nodeIndex) -> void {
	auto& parent = node(nodeIndex);
	if (parent.isLeaf())
		return;

	parent.childrenCrossChecked = false;
	clearCrossedFlag(parent.children[0]);
	clearCrossedFlag(parent.children[1]);
}

auto BvhCollisionSystem::collide(CollisionMap& collisions, const IgnoredCollisions& collisionsToIgnore, u32 nodeA, u32 nodeB) -> void {
	auto& a = node(nodeA);
	auto& b = node(nodeB);

	if (a.isLeaf() && b.isLeaf()) {
		auto aBody = ent.body.get(a.body);
		auto bBody = ent.body.get(b.body);
		if (!aBody.has_value() || !bBody.has_value()) {
			ASSERT_NOT_REACHED();
			return;

		}
		if (aBody->isStatic() && bBody->isStatic())
			return;

		BodyPair key{ a.body, b.body };

		if (collisionsToIgnore.contains(key))
			return;

		if (a.aabb.collides(b.aabb)) {

			Body* keyA;
			Body* keyB;
			// If the body pair swapped the ids then also swap the order so the collision normal and other things point in the correct direction.
			if (key.a == a.body) {
				keyA = &*aBody;
				keyB = &*bBody;
			} else {
				keyA = &*bBody;
				keyB = &*aBody;
			}
			if (auto collision = ::collide(keyA->transform, keyA->collider, keyB->transform, keyB->collider); collision.has_value()) {
				// TODO: Move this into some function or constructor probably when making a better collision system.
				collision->coefficientOfFriction = sqrt(keyA->coefficientOfFriction * keyB->coefficientOfFriction);
				collisions[key] = *collision;
			}
		}
	} else if (!a.isLeaf() && !b.isLeaf()) {
		if (a.aabb.collides(b.aabb)) {
			collide(collisions, collisionsToIgnore, a.children[0], b.children[0]);
			collide(collisions, collisionsToIgnore, a.children[0], b.children[1]);
			collide(collisions, collisionsToIgnore, a.children[1], b.children[0]);
			collide(collisions, collisionsToIgnore, a.children[1], b.children[1]);
		}
		if (!a.childrenCrossChecked) {
			collide(collisions, collisionsToIgnore, a.children[0], a.children[1]);
			a.childrenCrossChecked = true;
		}
		if (!b.childrenCrossChecked) {
			collide(collisions, collisionsToIgnore, b.children[0], b.children[1]);
			b.childrenCrossChecked = true;
		}
	} else if (a.isLeaf() && !b.isLeaf()) {
		if (a.aabb.collides(b.aabb)) {
			collide(collisions, collisionsToIgnore, nodeA, b.children[0]);
			collide(collisions, collisionsToIgnore, nodeA, b.children[1]);
		}
		if (!b.childrenCrossChecked) {
			collide(collisions, collisionsToIgnore, b.children[0], b.children[1]);
			b.childrenCrossChecked = true;
		}
	} else if (!a.isLeaf() && b.isLeaf()) {
		if (a.aabb.collides(b.aabb)) {
			collide(collisions, collisionsToIgnore, nodeB, a.children[0]);
			collide(collisions, collisionsToIgnore, nodeB, a.children[1]);
		}
		if (!a.childrenCrossChecked) {
			collide(collisions, collisionsToIgnore, a.children[0], a.children[1]);
			a.childrenCrossChecked = true;
		}
	}
}

auto BvhCollisionSystem::addMarginToAabb(const Aabb& aabb) -> Aabb {
	static constexpr float AABB_MARGIN = 0.2f;
	return Aabb{ aabb.min - Vec2{ AABB_MARGIN }, aabb.max + Vec2{ AABB_MARGIN } };
}

auto BvhCollisionSystem::insert(BodyId bodyId) -> void {

	auto aabb = [](const Body& body) -> Aabb {
		const auto aabb = ::aabb(body.collider, body.transform);
		return addMarginToAabb(aabb);
	};

	const auto& body = ent.body.get(bodyId);

	if (!body.has_value()) {
		ASSERT_NOT_REACHED();
		return;
	}

	if (rootNode == NULL_NODE) {
		rootNode = allocateNode();
		auto& root = node(rootNode);
		root = {
			.parent = NULL_NODE,
			.body = bodyId,
			.aabb = aabb(*body),
		};
		leafNodes.push_back(rootNode);
		root.children[0] = NULL_NODE;
		
	} else {
		const auto newNode = allocateNode();
		node(newNode) = {
			.children = 0,
			.body = bodyId,
			.aabb = aabb(*body),
		};
		node(newNode).children[0] = NULL_NODE;
		leafNodes.push_back(newNode);
		rootNode = insertHelper(rootNode, newNode);
	}
}

// Returns the new parent node. Can't just pass a reference because it might get invalidated.
auto BvhCollisionSystem::insertHelper(u32 parentNode, u32 nodeToInsert) -> u32 {
	if (node(parentNode).isLeaf()) {
		const auto oldNode = parentNode;

		parentNode = allocateNode();
		auto& newParent = node(parentNode);
		newParent.children[0] = oldNode;
		newParent.children[1] = nodeToInsert;
		newParent.aabb = node(newParent.children[0]).aabb.combined(node(newParent.children[1]).aabb);

		auto& old = node(oldNode);
		auto& inserted = node(nodeToInsert);
		if (old.parent == NULL_NODE) {
			rootNode = parentNode;
		}
		newParent.parent = old.parent;
		old.parent = parentNode;
		inserted.parent = parentNode;
		return parentNode;
	} else {
		auto& toInsert = node(nodeToInsert);
		auto& a = node(node(parentNode).children[0]);
		auto& b = node(node(parentNode).children[1]);

		const auto
			aAreaIncrease = a.aabb.combined(toInsert.aabb).area() - a.aabb.area(),
			bAreaIncrease = b.aabb.combined(toInsert.aabb).area() - b.aabb.area();

		// @Performance: Could use the sum of the area increases as the cost function also describec by Erin Catto.
		if (aAreaIncrease < bAreaIncrease)
			node(parentNode).children[0] = insertHelper(node(parentNode).children[0], nodeToInsert);
		else 
			node(parentNode).children[1] = insertHelper(node(parentNode).children[1], nodeToInsert);

		node(parentNode).aabb = node(node(parentNode).children[0]).aabb.combined(node(node(parentNode).children[1]).aabb);
		// @Performance: Could also rotate the tree like described in the presentation by Erin Catto at GDC 2019.
		return parentNode;
	}
}

// Doesn't free the node and doesn't remove the node from leaf nodes. This is because this function is also used to remove and reinsert nodes.
auto BvhCollisionSystem::removeLeafNode(u32 nodeToRemove) -> void {
	if (nodeToRemove == rootNode) {
		rootNode = NULL_NODE;
		return;
	}

	const auto& toRemove = node(nodeToRemove);

	auto& parent = node(toRemove.parent);
	const auto toRemovesSibling = parent.children[0] == nodeToRemove ? parent.children[1] : parent.children[0];

	if (parent.parent == NULL_NODE) {
		rootNode = toRemovesSibling;
		node(toRemovesSibling).parent = NULL_NODE;
	} else {
		auto& parentsParent = node(parent.parent);
		if (parentsParent.children[0] == toRemove.parent) {
			parentsParent.children[0] = toRemovesSibling;
		}
		else {
			parentsParent.children[1] = toRemovesSibling;
		}
		node(toRemovesSibling).parent = parent.parent;
	}
	freeNode(toRemove.parent);
}

auto BvhCollisionSystem::allocateNode() -> u32 {
	if (freeNodes.size() > 0) {
		const auto node = freeNodes.back();
		freeNodes.pop_back();
		return node;
	}

	nodes.push_back(Node{ .aabb = Aabb{ Vec2{ 0.0f }, Vec2{ 0.0f } } });
	return static_cast<u32>(nodes.size() - 1);
}

auto BvhCollisionSystem::freeNode(u32 index) -> void {
	// For debugging uninitialized or reused memory bugs.
	//node(index).children[0] = NULL_NODE - 1;
	//node(index).children[1] = NULL_NODE - 1;
	//node(index).parent = NULL_NODE - 1;
	freeNodes.push_back(index);
}

auto BvhCollisionSystem::node(u32 index) -> Node& {
	return nodes[index];
}

auto BvhCollisionSystem::node(u32 index) const -> const Node& {
	return nodes[index];
}

auto BvhCollisionSystem::debugPrint(u32 rootNodeIndex) -> void {
	put("%d\n", rootNodeIndex);

	if (node(rootNodeIndex).isLeaf()) {
		return;
	}
	debugPrintHelper(rootNodeIndex, 1);
}

auto BvhCollisionSystem::debugPrintHelper(u32 rootNodeIndex, i32 depth) -> void {
	if (node(rootNodeIndex).isLeaf()) {
		return;
	}

	auto indent = [&depth]() {
		for (i32 i = 0; i < depth; i++) put(" ");
	};

	indent();
	put("%d\n", node(rootNodeIndex).children[0]);
	debugPrintHelper(node(rootNode).children[0], depth + 1);
	indent();
	put("%d\n", node(rootNodeIndex).children[1]);
	debugPrintHelper(node(rootNodeIndex).children[1], depth + 1);
}

auto BvhCollisionSystem::debugDrawAabbs(u32 rootNodeIndex, i32 depth) -> void {
	auto color = Vec3::RED;

	auto& root = node(rootNodeIndex);
	if (!root.isLeaf())
		color = (depth % 2 == 0) ? Vec3::GREEN : Vec3::BLUE;

	if (root.isLeaf()) {
		Debug::drawAabb(root.aabb, color);
		return;
	}

	debugDrawAabbs(root.children[0], depth + 1);
	debugDrawAabbs(root.children[1], depth + 1);

	Debug::drawAabb(Aabb{ root.aabb.min - Vec2{ 0.03f }, root.aabb.max + Vec2{ 0.03f } }, color);
}
