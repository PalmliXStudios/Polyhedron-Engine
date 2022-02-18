///***
//*
//*	License here.
//*
//*	@file
//*
//*	Machinegun weapon.
//*
//***/
//#pragma once
//
//class SVGBaseEntity;
//class SVGBaseItem;
//
//class ItemWeaponMachinegun : public SVGBaseItem {
//public:
//    // Constructor/Deconstructor.
//    ItemWeaponMachinegun(Entity* svEntity, const std::string& displayString, uint32_t identifier);
//    virtual ~ItemWeaponMachinegun();
//
//    DefineItemMapClass("Machinegun", ItemIdentifier::WeaponMachinegun, "item_weapon_Machinegun", ItemWeaponMachinegun, SVGBaseItem);
//
//    // Item flags
//    //static constexpr int32_t IF_xxx = 1 << 0;
//    //static constexpr int32_t IF_xxx = 1 << 1;
//
//    //
//    // Interface functions.
//    //
//    virtual void Precache() override;	// Precaches data.
//    virtual void Spawn() override;	// Spawns the entity.
//    virtual void Respawn() override;	// Respawns the entity.
//    virtual void PostSpawn() override;	// PostSpawning is for handling entity references, since they may not exist yet during a spawn period.
//    virtual void Think() override;	// General entity thinking routine.
//
//    //
//    // Callback Functions.
//    //
//    //void HealthMegaUse( SVGBaseEntity* caller, SVGBaseEntity* activator );
//    void WeaponMachinegunThink(void);
//    //void HealthMegaDie(SVGBaseEntity* inflictor, SVGBaseEntity* attacker, int damage, const vec3_t& point);
//    //void HealthMegaTouch(SVGBaseEntity* self, SVGBaseEntity* other, cplane_t* plane, csurface_t* surf);
//    qboolean WeaponMachinegunPickup(SVGBaseEntity* other);
//
//private:
//};
