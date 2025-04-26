
#include <cassert>
#include <cmath>
#include <array>

#include "../framework/scene.hpp"
#include "../framework/game.hpp"
#include "../framework/engine.hpp"

//-------------------------------------------------------
//	Basic Vector2 class
//-------------------------------------------------------

class Vector2
{
public:
	float x = 0.f;
	float y = 0.f;

	constexpr Vector2() = default;
	constexpr Vector2( float vx, float vy );
	constexpr Vector2( Vector2 const &other ) = default;

	float length() const;
	void normalize();
	void invertY();
	void invertX();
};


constexpr Vector2::Vector2( float vx, float vy ) :
	x( vx ),
	y( vy )
{
}

float Vector2::length() const
{
	return pow(x * x + y * y, 0.5);
}

void Vector2::normalize() 
{
	float len = this->length();
	x /= len;
	y /= len;
}

void Vector2::invertY()
{
	y *= -1;
}

void Vector2::invertX()
{
	x *= -1;
}

//-------------------------------------------------------
//	game parameters
//-------------------------------------------------------

namespace Params
{
	namespace System
	{
		constexpr int targetFPS = 60;
	}

	namespace Table
	{
		constexpr float width = 15.f;
		constexpr float height = 8.f;
		constexpr float pocketRadius = 0.5f;

		static constexpr std::array< Vector2, 6 > pocketsPositions =
		{
			Vector2{ -0.5f * width, -0.5f * height },
			Vector2{ 0.f, -0.5f * height },
			Vector2{ 0.5f * width, -0.5f * height },
			Vector2{ -0.5f * width, 0.5f * height },
			Vector2{ 0.f, 0.5f * height },
			Vector2{ 0.5f * width, 0.5f * height }
		};

		static constexpr std::array< Vector2, 7 > ballsPositions =
		{
			// player ball
			Vector2( -0.3f * width, 0.f ),
			// other balls
			Vector2( 0.2f * width, 0.f ),
			Vector2( 0.25f * width, 0.05f * height ),
			Vector2( 0.25f * width, -0.05f * height ),
			Vector2( 0.3f * width, 0.1f * height ),
			Vector2( 0.3f * width, 0.f ),
			Vector2( 0.3f * width, -0.1f * height )
		};
	}

	namespace Ball
	{
		constexpr float radius = 0.3f;
	}

	namespace Shot
	{
		constexpr float chargeTime = 1.f;
	}
}

//-------------------------------------------------------
//	Table logic
//-------------------------------------------------------

class Table
{
public:
	Table() = default;
	Table(Table const&) = delete;

	void init();
	void deinit();

	float speedSum() const;

	std::array< Vector2, 7 > ballsCurrentPositions = {};
	std::array< Vector2, 7 > speedDirection = {};
	std::array < float, 7 > speedModulus = {};
	std::array< bool, 7 > isPocketed = {};
	std::array< Scene::Mesh*, 7 > balls = {};
private:
	std::array< Scene::Mesh*, 6 > pockets = {};
};


void Table::init()
{
	ballsCurrentPositions = Params::Table::ballsPositions;
	isPocketed.fill( 0 );

	for ( int i = 0; i < 6; i++ )
	{
		assert( !pockets[ i ] );
		pockets[ i ] = Scene::createPocketMesh( Params::Table::pocketRadius );
		Scene::placeMesh( pockets[ i ], Params::Table::pocketsPositions[ i ].x, Params::Table::pocketsPositions[ i ].y, 0.f );
	}

	for ( int i = 0; i < 7; i++ )
	{
		assert( !balls[ i ] );
		balls[ i ] = Scene::createBallMesh( Params::Ball::radius );
		Scene::placeMesh( balls[ i ], Params::Table::ballsPositions[ i ].x, Params::Table::ballsPositions[ i ].y, 0.f );

		speedDirection[ i ] = { 0, 0 };
		speedModulus[ i ] = 0.f;
	}
}


void Table::deinit()
{
	for ( Scene::Mesh* mesh : pockets )
		Scene::destroyMesh( mesh );

	for ( Scene::Mesh* mesh : balls )
	{
		if ( mesh )
			Scene::destroyMesh( mesh );
	}

	ballsCurrentPositions = {};
	speedDirection = {};
	speedModulus = {};
	isPocketed = {};
	pockets = {};
	balls = {};
}

float Table::speedSum() const
{	
	float sum = 0;
	for ( auto v : speedModulus )
	{
		sum += v;
	}
	return sum;
}

//-------------------------------------------------------
//	game public interface
//-------------------------------------------------------

namespace Game
{
	Table table;

	bool isChargingShot = false;
	float shotChargeProgress = 0.f;

	bool isBallsMoving = false;

	void init()
	{
		Engine::setTargetFPS( Params::System::targetFPS );
		Scene::setupBackground( Params::Table::width, Params::Table::height );
		table.init();
	}


	void deinit()
	{
		table.deinit();
	}

	bool isInPocket( int ballIdx, Scene::Mesh* ball )
	{
		float x = table.ballsCurrentPositions[ ballIdx ].x;
		float y = table.ballsCurrentPositions[ ballIdx ].y;

		for ( int i = 0; i < 6; i++ )
		{
			Vector2 distance = { Params::Table::pocketsPositions[ i ].x - x,
								 Params::Table::pocketsPositions[ i ].y - y };
			if ( distance.length() <= Params::Table::pocketRadius ) 
			{
				return true;
			}
		}
		return false;
	}

	void checkBorders( int ballIdx )
	{
		float& x = table.ballsCurrentPositions[ ballIdx ].x;
		float& y = table.ballsCurrentPositions[ ballIdx ].y;
		Vector2& speedDirection = table.speedDirection[ ballIdx ];
		float& speedModulus = table.speedModulus[ ballIdx ];

		if ( y < -0.5f * Params::Table::height + Params::Ball::radius )
		{
			y = -0.5f * Params::Table::height + Params::Ball::radius;
			speedModulus -= 0.15f * speedModulus * ( 1.f + abs( speedDirection.y ) );
			speedDirection.invertY();
		}

		if ( y > 0.5f * Params::Table::height - Params::Ball::radius )
		{
			y = 0.5f * Params::Table::height - Params::Ball::radius;
			speedModulus -= 0.15f * speedModulus * ( 1.f + speedDirection.y );
			speedDirection.invertY();
		}

		if ( x < -0.5f * Params::Table::width + Params::Ball::radius )
		{
			x = -0.5f * Params::Table::width + Params::Ball::radius;
			speedModulus -= 0.15f * speedModulus * ( 1.f + abs( speedDirection.x ) );
			speedDirection.invertX();
		}

		if ( x > 0.5f * Params::Table::width - Params::Ball::radius )
		{
			x = 0.5f * Params::Table::width - Params::Ball::radius;
			speedModulus -= 0.15f * speedModulus * ( 1.f + speedDirection.x );
			speedDirection.invertX();
		}
	}

	void checkBallCollision( int ballIdx )
	{
		float& x = table.ballsCurrentPositions[ ballIdx ].x;
		float& y = table.ballsCurrentPositions[ ballIdx ].y;
		Vector2& speedDirection = table.speedDirection[ ballIdx ];
		float& speedModulus = table.speedModulus[ ballIdx ];

		for ( int i = 0; i < 7; i++ )
		{
			if ( i == ballIdx )
				continue;
			Vector2 distance = { table.ballsCurrentPositions[ i ].x - x,
								 table.ballsCurrentPositions[ i ].y - y };
			float len = distance.length();
			if ( len < 2 * Params::Ball::radius )
			{
				float s = distance.x / len; //sin
				float c = distance.y / len; //cos

				distance.normalize();
				x -= distance.x * ( 2 * Params::Ball::radius - len );
				y -= distance.y * ( 2 * Params::Ball::radius - len );

				float vn1 = speedDirection.x * speedModulus * s
					+ speedDirection.y * speedModulus * c;
				float vn2 = table.speedDirection[ i ].x * table.speedModulus[ i ] * s
					+ table.speedDirection[ i ].y * table.speedModulus[ i ] * c;
				float vt1 = -table.speedDirection[ i ].x * table.speedModulus[ i ] * c
					+ table.speedDirection[ i ].y * table.speedModulus[ i ] * s;
				float vt2 = -speedDirection.x * speedModulus * c
					+ speedDirection.y * speedModulus * s;

				speedDirection.x = 0.85f * ( vn2 * s - vt2 * c ) + 0.15f * ( vn1 * s - vt1 * c );
				speedDirection.y = 0.85f * ( vn2 * c + vt2 * s ) + 0.15f * ( vn1 * c + vt1 * s );
				table.speedDirection[ i ].x = 0.85 * ( vn1 * s - vt1 * c ) + 0.15f * ( vn2 * s - vt2 * c );
				table.speedDirection[ i ].y = 0.85 * ( vn1 * c + vt1 * s ) + 0.15f * ( vn2 * c + vt2 * s );
				
				speedModulus = 0.95f * speedDirection.length();
				speedDirection.normalize();

				table.speedModulus[ i ] = 0.95f * table.speedDirection[ i ].length();
				table.speedDirection[ i ].normalize();
			}
		}
	}

	void moveBall( int ballIdx, float dt )
	{
		float& x = table.ballsCurrentPositions[ ballIdx ].x;
		float& y = table.ballsCurrentPositions[ ballIdx ].y;
		Vector2& speedDirection = table.speedDirection[ ballIdx ];
		float& speedModulus = table.speedModulus[ ballIdx ];
		Scene::Mesh* ball = table.balls[ ballIdx ];

		if ( isInPocket( ballIdx, ball ) )
		{	
			if ( !ballIdx )
			{
				deinit();
				init();
				return;
			}
			Scene::destroyMesh( ball );
			table.balls[ ballIdx ] = nullptr;
			x = 2 * Params::Table::width;
			y = x;
			speedModulus = 0;
			table.isPocketed[ ballIdx ] = true;
			return;
		}

		checkBorders( ballIdx );
		checkBallCollision( ballIdx );

		float newX = x + speedDirection.x * speedModulus * dt;
		float newY = y + speedDirection.y * speedModulus * dt;
		x = newX;
		y = newY;

		Scene::placeMesh( ball, x, y, 0.f );
		speedModulus = std::max( speedModulus - 0.05f * Params::Table::width * dt, 0.f );
	}

	void update( float dt )
	{
		if ( isChargingShot )
			shotChargeProgress = std::min( shotChargeProgress + dt / Params::Shot::chargeTime, 1.f );
		Scene::updateProgressBar( shotChargeProgress );

		if ( isBallsMoving ) 
		{		
			for ( int i = 0; i < 7; i++ )
			{
				if ( !table.isPocketed[ i ] )
					moveBall( i, dt );
			}
			if ( !table.speedSum() )
				isBallsMoving = false;
		}
	}

	void mouseButtonPressed( float x, float y )
	{
		if ( !isBallsMoving )
			isChargingShot = true;
	}

	void mouseButtonReleased( float x, float y )
	{
		if ( !isBallsMoving )
		{
			table.speedDirection[ 0 ] = { x - table.ballsCurrentPositions[ 0 ].x,
										  y - table.ballsCurrentPositions[ 0 ].y };
			table.speedDirection[ 0 ].normalize();
			table.speedModulus[ 0 ] = shotChargeProgress * Params::Table::width;

			isBallsMoving = true;
                                                                               
			isChargingShot = false;
			shotChargeProgress = 0.f;
		}
	}
}
