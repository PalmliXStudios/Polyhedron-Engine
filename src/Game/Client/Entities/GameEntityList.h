/***
*
*	License here.
*
*	@file
*
*	Client Game BaseEntity.
* 
***/
#pragma once


// Predeclaration(s).
class CLGBasePacketEntity;

/**
*
*   Manages allocation and destruction of client game class entities.
* 
*   The first 2048 slots are reserved strictly for server entities only.
*   The second 2048 slots are reserved for client side only entities.
*   
*   Since we're dealing with incoming packets telling us the state of each entity in frame, 
*   we'll have to carefully manage which states belong to what entities. The entity classname
*   is send over the wire as a hash value(Com_HashString). Whenever it changes, the current
*   class object here is destroyed. A newly allocated class object which has a registered hash
*   that matches the received hashstring takes its place instead. 
*
*	If it fails to find an equal hashstring however, it'll resort to the default CLGBasePacketEntity
*	instead. The default CLGBasePacketEntity takes care of the default behavior. (Similar to how it
*	would be in Quake 2 without having control over client-side entities.)
* 
*   In similar fashion like the SVGBaseEntity you have Set, Get, callback and think functions
*   respectively.
* 
**/
using CLGEntityVector = std::vector<IClientGameEntity*>;

class GameEntityList {
public:
    //! Constructor/Destructor.
    GameEntityList() {
        // 2048 for server entities, and 2048 more for 'local' client side only entities.
        gameEntities.reserve(MAX_CLIENT_POD_ENTITIES);

        // To ensure that slot 0 is in use, keeps indexes correct.
        gameEntities.push_back(nullptr);
    }
    virtual ~GameEntityList() = default;

    /**
    *   @brief  Clears the list by deallocating all its members.
    **/
    void Clear();

	/**
	*   @brief  Spawns and inserts a new game entity of type 'classname', which belongs to the PODEntity.
	*   @return Pointer to the game entity object on sucess. On failure, nullptr.
	**/
	GameEntity *AllocateFromClassname(const std::string &classname, PODEntity* clEntity);

    /**
    *   @brief  Spawns and inserts a new game entity at the state.number index, assigning the client entity pointer
    *           as its 'soulmate', I suppose.
    *   @return Pointer to the game entity object on sucess. On failure, nullptr.
    **/
    GameEntity *UpdateGameEntityFromState(const EntityState &state, PODEntity *clEntity);

    /**
    *   @return A pointer to the entity who's index matches the state number.
    **/
    GameEntity *GetByNumber(int32_t number);

    /**
    *   @brief  Inserts the game entity pointer at the number index of our game entity vector.
    *   @param  force   When set to true it'll delete any previously allocated game entity occupying the given slot.
    *   @return Pointer to the entity being inserted. nullptr on failure.
    **/
    GameEntity *InsertAt(int32_t number, IClientGameEntity *clgEntity, bool force = true);

	// Return pointer to game entity vector.
    inline GameEntityVector &GetGameEntities() { return gameEntities; };

private:
    //! First 2048 are reserved for server side entities.
    GameEntityVector gameEntities;
};