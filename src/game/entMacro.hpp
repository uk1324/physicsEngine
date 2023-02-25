#define ENTITY_TYPE_LIST(macro, separator) \
	macro(Body, body) separator \
	macro(DistanceJoint, distanceJoint) separator \
	macro(RevoluteJoint, revoluteJoint) separator \
	macro(Trail, trail)

#define COMMA ,
#define ENTITY_TYPE_LIST_COMMA_SEPARATED(macro) ENTITY_TYPE_LIST(macro, COMMA)
