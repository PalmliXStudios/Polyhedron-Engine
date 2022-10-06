/***
*
*	License here.
*
*	@file
*
*	ServerGame BaseMover Entity.
* 
***/
//! Main Headers.
#include "Game/Server/ServerGameMain.h"
//! Server Game Local headers.
#include "Game/Server/ServerGameLocals.h"

#include "../../Effects.h"
#include "../../Entities.h"
#include "../../Utilities.h"

#include "SVGBaseTrigger.h"
#include "SVGBaseMover.h"



// Epsilon used for comparing floats.
static constexpr float BASEMOVER_EPSILON = 0.03125;

// Constructor/Deconstructor.
SVGBaseMover::SVGBaseMover(PODEntity *svEntity) : Base(svEntity) {

}

/***
*
*
*	Linear Movement Mechanics.
*
*
***/
///**
//*	@brief 
//**/
//const int64_t SG_LinearMovement( const PODEntity *podEntity, const int64_t &time, vec3_t &dest ) {
//	// Relative move time.
//	int64_t moveTime = time - podEntity->linearMovementTimeStamp;
//	if( moveTime < 0 ) {
//		moveTime = 0;
//	}
//
//	if( podEntity->linearMovementDuration ) {
//		if( moveTime > (int)podEntity->linearMovementDuration ) {
//			moveTime = podEntity->linearMovementDuration;
//		}
//
//		const vec3_t dist = podEntity->linearMovementEndOrigin - podEntity->linearMovementBeginOrigin;
//		double moveFrac = (double)moveTime / (double)podEntity->linearMovementDuration;
//		moveFrac = Clampf( moveFrac, 0., 1. );
//		dest = vec3_fmaf( podEntity->linearMovementBeginOrigin, moveFrac, dist );
//	} else {
//		double moveFrac = moveTime * 0.001;
//		dest = vec3_fmaf( podEntity->linearMovementBeginOrigin, moveFrac, podEntity->linearMovementVelocity );
//	}
//
//	return moveTime;
//}
//
///**
//*	@brief 
//**/
//void SG_LinearMovementDelta( const PODEntity *podEntity, const int64_t &oldTime, const int64_t &curTime, vec3_t &dest ) {
//	vec3_t oldTimeOrigin		= vec3_zero();
//	vec3_t currentTimeOrigin	= vec3_zero();
//	SG_LinearMovement( podEntity, oldTime, oldTimeOrigin );
//	SG_LinearMovement( podEntity, curTime, currentTimeOrigin );
//	dest = currentTimeOrigin - oldTimeOrigin;
//}



/***
*
*
*	Linear "BaseMover", currently static functions, will be moved to CLGLinearBaseMover later on.
*
*
***/
void LinearMove_UpdateLinearVelocity( SVGBaseMover *geMover, float dist, const int32_t speed ) {
	// Maintain sanity.
	if ( !geMover ) {
		return;
	}

	// Move duration.
	int32_t duration = 0;
	
	// Calculate duration in seconds based on distance and speed.
	if( speed ) {
		duration = static_cast<float>(dist * 1000.0f / speed);
		if( !duration ) {
			duration = 1;
		}
	}

	// Setup the linear movement state properties.
	PODEntity *podEntity = geMover->GetPODEntity();
	podEntity->linearMovement = ( speed != 0 ? true : false );
	if( !podEntity->linearMovement ) {
		return;
	}

	podEntity->linearMovementEndOrigin = geMover->GetPushMoveInfo()->destOrigin;
	podEntity->linearMovementBeginOrigin = geMover->GetOrigin();
	podEntity->linearMovementTimeStamp = ( level.time /*- FRAMERATE_MS */).count();
	podEntity->linearMovementDuration = duration;
}

void LinearMove_Done( SVGBaseMover *geMover ) {
	// Maintain sanity.
	if ( !geMover ) {
		return;
	}

	geMover->SetVelocity( vec3_zero() );
	//ent->moveinfo.endfunc( ent );
	geMover->GetPushMoveInfo()->OnEndFunction( geMover );
	geMover->DispatchStopCallback(); //G_CallStop( ent );

	//LinearMove_UpdateLinearVelocity( geMover, 0, 0 );
}

void LinearMove_Watch( SVGBaseMover *geMover ) {
	// Maintain sanity.
	if ( !geMover ) {
		return;
	}

	// Get POD Entity.
	PODEntity *podEntity = geMover->GetPODEntity();

	int32_t moveTime = level.time.count() - podEntity->linearMovementTimeStamp; //game.serverTime - ent->s.linearMovementTimeStamp;
	
	if( moveTime >= static_cast<int32_t>(podEntity->linearMovementDuration) ) {
		geMover->SetThinkCallback( &SVGBaseMover::LinearBrushMoveDone ); //ent->think = LinearMove_Done;
		geMover->SetNextThinkTime( level.time + FRAMERATE_MS ); // ent->nextThink = level.time + 1;
		//geMover->LinearBrushMoveDone();
		return;
	}

	geMover->SetThinkCallback( &SVGBaseMover::LinearBrushMoveWatch ); ////	ent->think = LinearMove_Watch;
	geMover->SetNextThinkTime( level.time + FRAMERATE_MS ); //	ent->nextThink = level.time + 1;
}

void LinearMove_Begin( SVGBaseMover *geMover ) {
	// Maintain sanity.
	if ( !geMover ) {
		return;
	}

	// set up velocity vector
	const vec3_t dir = geMover->GetPushMoveInfo()->destOrigin - geMover->GetOrigin(); //VectorSubtract( ent->moveinfo.dest, ent->s.origin, dir );
	float dist = 0.;
	vec3_normalize_length( dir, dist );
	geMover->SetVelocity( vec3_scale( dir, geMover->GetPushMoveInfo()->speed ) ); // VectorScale( dir, ent->moveinfo.speed, ent->velocity );

	geMover->SetThinkCallback( &SVGBaseMover::LinearBrushMoveWatch ); ////	ent->think = LinearMove_Watch;
	geMover->SetNextThinkTime( level.time + FRAMERATE_MS ); //	ent->nextThink = level.time + 1;

	LinearMove_UpdateLinearVelocity( geMover, dist, geMover->GetPushMoveInfo()->speed );
}

void LinearMove_Calc( SVGBaseMover *geMover, const vec3_t &dest, PushMoveEndFunction *pushMoveEndFunction ) {
	// Maintain sanity.
	if ( !geMover ) {
		return;
	}

	// Unset its velocity.
	geMover->SetVelocity( vec3_zero() );

	// Update move info destination.
	geMover->GetPushMoveInfo()->destOrigin = dest;
	geMover->GetPushMoveInfo()->OnEndFunction = pushMoveEndFunction;
	LinearMove_UpdateLinearVelocity( geMover, 0, 0 );

	if( level.currentEntity == ( ( geMover->GetFlags() & EntityFlags::TeamSlave) ? geMover->GetTeamMasterEntity() : geMover ) ) {
		LinearMove_Begin( geMover );
		//geMover->SetThinkCallback( &SVGBaseMover::LinearBrushMoveBegin );
		//geMover->SetNextThinkTime( level.time + FRAMERATE_MS );
	} else {
		geMover->SetThinkCallback( &SVGBaseMover::LinearBrushMoveBegin );
		geMover->SetNextThinkTime( level.time + FRAMERATE_MS + FRAMERATE_MS );
	}
}

void SVGBaseMover::LinearBrushMoveBegin() {
	LinearMove_Begin( this );
}
void SVGBaseMover::LinearBrushMoveWatch() {
	LinearMove_Watch( this );
}
void SVGBaseMover::LinearBrushMoveDone() {
	LinearMove_Done( this );
}

/**
*	@brief	Additional spawnkeys for movers.
**/
void SVGBaseMover::SpawnKey(const std::string& key, const std::string& value) {
	// Accel.
	if (key == "accel") {
		// Parse float.
		float parsedFloat = 0.f;
		ParseKeyValue(key, value, parsedFloat);

		// Set acceleration.
		SetAcceleration(parsedFloat);
	}
	// Accel.
	else if (key == "decel") {
		// Parse float.
		float parsedFloat = 0.f;
		ParseKeyValue(key, value, parsedFloat);

		// Set deceleration.
		SetDeceleration(parsedFloat);
	}
	// Lip.
	else if (key == "lip") {
		// Parse float.
		float parsedFloat = 0.f;
		ParseKeyValue(key, value, parsedFloat);

		// Set lip.
		SetLip(parsedFloat);
	}
	// Speed.
	else if (key == "speed") {
		// Parse float.
		float parsedFloat = 0.f;
		ParseKeyValue(key, value, parsedFloat);

		// Set speed.
		SetSpeed(parsedFloat);
	}
	// Wait.
	else if (key == "wait") {
		// Set wait.
		//SetWaitTime(parsedFloat);
		ParseKeyValue(key, value, waitTime);
	} else {
		Base::SpawnKey(key, value);
	}
}

//
//===============
// SVGBaseMover::SetMoveDirection
//
//===============
//
void SVGBaseMover::SetMoveDirection(const vec3_t& angles, const bool resetAngles) {
	const vec3_t VEC_UP = { 0, -1.f, 0 };
	const vec3_t MOVEDIR_UP = { 0, 0, 1.f };
	const vec3_t VEC_DOWN = { 0, -2.f, 0 };
	const vec3_t MOVEDIR_DOWN = { 0, 0, -1.f };

	const vec3_t AnglesUp[] = { 
		{ 90.f, 0, 0 }, 
		{ -270.f, 0, 0 } 
	};
	
	const vec3_t AnglesDown[] = { 
		{ 270.f, 0, 0 }, 
		{ -90.f, 0, 0 } 
	};

	if (vec3_equal(angles, VEC_UP) || vec3_equal(angles, AnglesUp[0]) || vec3_equal(angles, AnglesUp[1])) {
		moveDirection = MOVEDIR_UP;
	} else if (vec3_equal(angles, VEC_DOWN) || vec3_equal(angles, AnglesDown[0]) || vec3_equal(angles, AnglesDown[1])) {
		moveDirection = MOVEDIR_DOWN;
	} else {
		AngleVectors(angles, &moveDirection, NULL, NULL);
	}

	// Admer: is this really intended? Some entities may control their angle
	// and align it directly with the movement direction.
	// I suggest we add a bool parameter to this method, 'resetAngles',
	// which will zero the entity's angles if true
	if (resetAngles == true) {
		SetAngles(vec3_zero());
	}
}

//===============
// SVGBaseMover::CalculateEndPosition
//===============
vec3_t SVGBaseMover::CalculateEndPosition() {
	const vec3_t& size = GetSize();
	vec3_t absoluteDir{
		fabsf( moveDirection.x ),
		fabsf( moveDirection.y ),
		fabsf( moveDirection.z ) 
	};
	
	moveInfo.distance = (absoluteDir.x * size.x) + (absoluteDir.y * size.y) + (absoluteDir.z * size.z) - GetLip();

	return vec3_fmaf( GetStartPosition(), moveInfo.distance, moveDirection );
}

//===============
// SVGBaseMover::SwapPositions
//===============
void SVGBaseMover::SwapPositions() {
	vec3_t temp = GetEndPosition();			// temp = endpos
	SetEndPosition( GetStartPosition() );	// endpos = startpos
	SetStartPosition( temp );				// startpos = temp
}

// =========================
// SVGBaseMover::BrushMoveDone
// =========================
void SVGBaseMover::BrushMoveDone() {
	SetVelocity( vec3_zero() );
	moveInfo.OnEndFunction( this );
}

//===============
// SVGBaseMover::BrushMoveFinal
//===============
void SVGBaseMover::BrushMoveFinal() {
	// We've traveled our world, time to rest
	if ( moveInfo.remainingDistance == 0.0f ) {
		BrushMoveDone();
		return;
	}

	// Move only as far as to clear the remaining distance
	SetVelocity( vec3_scale( moveInfo.dir, moveInfo.remainingDistance / FRAMETIME_S.count() ) );

	SetThinkCallback( &SVGBaseMover::BrushMoveDone );
	SetNextThinkTime( level.time + FRAMETIME_S );
}

//===============
// SVGBaseMover::BrushMoveBegin
//===============
void SVGBaseMover::BrushMoveBegin() {
	float frames;

	// It's time to stop
	if ( (moveInfo.speed * FRAMETIME_S.count()) >= moveInfo.remainingDistance ) {
		BrushMoveFinal();
		return;
	}

	SetVelocity( vec3_scale( moveInfo.dir, moveInfo.speed ) );

	frames = floor( (moveInfo.remainingDistance / moveInfo.speed) / FRAMETIME_S.count() );
	moveInfo.remainingDistance -= frames * moveInfo.speed * FRAMETIME_S.count();

	SetThinkCallback( &SVGBaseMover::BrushMoveFinal );
	SetNextThinkTime( level.time + frames * FRAMETIME_S);//Frametime(frames * FRAMETIME_S.count()) );
}

//===============
// SVGBaseMover::BrushMoveCalc
//===============
void SVGBaseMover::BrushMoveCalc( const vec3_t& destination, PushMoveEndFunction* function ) {
	PushMoveInfo& mi = moveInfo;

	SetVelocity( vec3_zero() );
	mi.dir = destination - GetOrigin();
	mi.remainingDistance = VectorNormalize( moveInfo.dir );
	mi.OnEndFunction = function;

	if ( EqualEpsilonf( mi.speed, mi.acceleration, BASEMOVER_EPSILON ) && EqualEpsilonf( mi.speed, mi.deceleration, BASEMOVER_EPSILON  ) ) {
		if ( level.currentEntity == ((GetFlags() & EntityFlags::TeamSlave) ? GetTeamMasterEntity() : this) ) {
			BrushMoveBegin();
		} else {
			SetThinkCallback( &SVGBaseMover::BrushMoveBegin );
			SetNextThinkTime( level.time + FRAMERATE_MS );
		}
	} else {
		// Accelerative movement
		mi.currentSpeed = 0;

		SetThinkCallback( &SVGBaseMover::BrushAccelerateThink );
		SetNextThinkTime( level.time + FRAMERATE_MS );
	}
}

//===============
// SVGBaseMover::BrushAngleMoveDone
//===============
void SVGBaseMover::BrushAngleMoveDone() {
	SetAngularVelocity( vec3_zero() );
	moveInfo.OnEndFunction( this );
}

//===============
// SVGBaseMover::BrushAngleMoveFinal
//===============
void SVGBaseMover::BrushAngleMoveFinal() {
	vec3_t move;

	if ( moveInfo.state == MoverState::Up ) {
		move = moveInfo.endAngles - GetAngles();
	} else {
		move = moveInfo.startAngles - GetAngles();
	}

	if ( vec3_equal( move, vec3_zero() ) ) {
		BrushAngleMoveDone();
		return;
	}

	SetAngularVelocity( vec3_scale( move, 1.0f / FRAMETIME_S.count() ) );

	SetThinkCallback( &SVGBaseMover::BrushAngleMoveDone );
	SetNextThinkTime( level.time + FRAMETIME_S );
}

//===============
// SVGBaseMover::BrushAngleMoveBegin
//===============
void SVGBaseMover::BrushAngleMoveBegin() {
	vec3_t destinationDelta;
	float length, travelTime, frames;

	// Set destinationDelta to the vector needed to move
	if ( moveInfo.state == MoverState::Up ) {
		destinationDelta = moveInfo.endAngles - GetAngles();
	} else {
		destinationDelta = moveInfo.startAngles - GetAngles();
	}

	// Get the length of destinationDelta to then get time to reach the destination
	length = vec3_length( destinationDelta );
	travelTime = length / moveInfo.speed;

	if ( travelTime < FRAMETIME_S.count() ) {
		BrushAngleMoveFinal();
		return;
	}

	frames = floor( travelTime / FRAMETIME_S.count() );

	// Get the velocity by scaling the delta vector by the time spent traveling
	SetAngularVelocity( vec3_scale( destinationDelta, 1.0f / travelTime ) );

	SetThinkCallback( &SVGBaseMover::BrushAngleMoveFinal );
	SetNextThinkTime( level.time + frames * FRAMETIME_S );
}

//===============
// SVGBaseMover::BrushAngleMoveCalc
//===============
void SVGBaseMover::BrushAngleMoveCalc( PushMoveEndFunction* function ) {
	SetAngularVelocity( vec3_zero() );
	moveInfo.OnEndFunction = function;

	if ( level.currentEntity == ((GetFlags() & EntityFlags::TeamSlave) ? GetTeamMasterEntity() : this) ) {
		BrushAngleMoveBegin();
	} else {
		SetThinkCallback( &SVGBaseMover::BrushAngleMoveBegin );
		SetNextThinkTime( level.time + FRAMERATE_MS );
	}
}

//===============
// SVGBaseMover::BrushAccelerateCalc
//===============
void SVGBaseMover::BrushAccelerateCalc() {
	float accelerationDistance, decelerationDistance;

	moveInfo.moveSpeed = moveInfo.speed;

	if ( moveInfo.remainingDistance < moveInfo.acceleration ) {
		moveInfo.currentSpeed = moveInfo.remainingDistance;
		return;
	}

	accelerationDistance = CalculateAccelerationDistance( moveInfo.speed, moveInfo.acceleration );
	decelerationDistance = CalculateAccelerationDistance( moveInfo.speed, moveInfo.deceleration );

	if ( (moveInfo.remainingDistance - accelerationDistance - decelerationDistance) < 0 ) {
		float factor = (moveInfo.acceleration + moveInfo.deceleration) / (moveInfo.acceleration * moveInfo.deceleration);
		moveInfo.moveSpeed = (-2.0f + sqrt(4.0f - 4.0f * factor * (-2.0f * moveInfo.remainingDistance))) / (2.0f * factor);
	}

	moveInfo.deceleratedDistance = decelerationDistance;
}

//===============
// SVGBaseMover::BrushAccelerate
//===============
void SVGBaseMover::BrushAccelerate() {
	// Are we decelerating?
	if ( moveInfo.remainingDistance <= moveInfo.deceleratedDistance ) {
		if ( moveInfo.remainingDistance < moveInfo.deceleratedDistance ) {
			if ( moveInfo.nextSpeed ) {
				moveInfo.currentSpeed = moveInfo.nextSpeed;
				moveInfo.nextSpeed = 0.0f;
				return;
			}
			if ( moveInfo.currentSpeed > moveInfo.deceleration ) {
				moveInfo.currentSpeed -= moveInfo.deceleration;
			}
		}
		return;
	}

	// Are we at full speed and need to start decelerating during this move?
	if ( moveInfo.currentSpeed == moveInfo.moveSpeed ) {
		if ( (moveInfo.remainingDistance - moveInfo.currentSpeed) < moveInfo.deceleratedDistance ) {
			float p1Distance, p2Distance, distance;

			p1Distance = moveInfo.remainingDistance - moveInfo.deceleratedDistance;
			p2Distance = moveInfo.moveSpeed * (1.0f - (p1Distance / moveInfo.moveSpeed));
			distance = p1Distance + p2Distance;
			moveInfo.currentSpeed = moveInfo.moveSpeed;
			moveInfo.nextSpeed = moveInfo.moveSpeed - moveInfo.deceleration * (p2Distance / distance);
			return;
		}
	}

	// Are we accelerating?
	if ( moveInfo.currentSpeed < moveInfo.speed ) {
		float p1Speed, oldSpeed;
		float p1Distance, p2Distance, distance;

		oldSpeed = moveInfo.currentSpeed;

		// Figure simple acceleration up to move speed
		moveInfo.currentSpeed += moveInfo.acceleration;
		if ( moveInfo.currentSpeed > moveInfo.speed ) {
			moveInfo.currentSpeed = moveInfo.speed;
		}

		// Are we accelerating throughout this entire move?
		if ( (moveInfo.remainingDistance - moveInfo.currentSpeed) >= moveInfo.deceleratedDistance ) {
			return;
		}

		// During this move we will accelerate from currentSpeed to moveSpeed
		// and cross over the deceleratedDistance; figure the average speed for the entire move
		p1Distance = moveInfo.remainingDistance - moveInfo.deceleratedDistance;
		p1Speed = (oldSpeed + moveInfo.moveSpeed) / 2.0f;
		p2Distance = moveInfo.moveSpeed * (1.0f - (p1Distance / p1Speed));
		distance = p1Distance + p2Distance;
		moveInfo.currentSpeed = (p1Speed * (p1Distance / distance)) + (moveInfo.moveSpeed * (p2Distance / distance));
		moveInfo.nextSpeed = moveInfo.moveSpeed - moveInfo.deceleration * (p2Distance / distance);
		return;
	}

	// We are at constant velocity (moveSpeed)
	return;
}

//===============
// SVGBaseMover::BrushAccelerateThink
// 
// The team has completed a frame of movement,
// so change the speed for the next frame
//===============
void SVGBaseMover::BrushAccelerateThink() {
	// Calculate remaining distance.
	moveInfo.remainingDistance -= moveInfo.currentSpeed;

	// In case of a start or blockade speed == 0 so recalculate acceleration.
	if ( moveInfo.currentSpeed == 0 ) {
		BrushAccelerateCalc();
	}

	// Accelerate.
	BrushAccelerate();

	// Will the entire move complete on the next frame
	if ( moveInfo.remainingDistance <= moveInfo.currentSpeed ) {
		BrushMoveFinal();
		return;
	}

	SetVelocity( vec3_scale( moveInfo.dir, moveInfo.currentSpeed * BASE_FRAMERATE ) );

	SetThinkCallback( &SVGBaseMover::BrushAccelerateThink );
	SetNextThinkTime( level.time + FRAMERATE_MS );
}

//===============
// SVGBaseMover::CalculateAccelerationDistance
//===============
float SVGBaseMover::CalculateAccelerationDistance( float targetSpeed, float accelerationRate ) {
	if ( accelerationRate == 0.0f ) {
		gi.DPrintf( "%s '%s': accelerationRate was 0!\n", 
					GetTypeInfo()->classname, 
					GetTargetName().empty() ? "unnamed" : GetTargetName().c_str() );
		return 0.0f;
	}
	return (targetSpeed * ((targetSpeed / accelerationRate) + 1.0f) / 2.0f);
}
