/***
*
*	License here.
*
*	@file
*
*	ClientGame BaseTrigger Entity.
* 
***/
#pragma once

class CLGBasePacketEntity;

class CLGBaseTrigger : public CLGBasePacketEntity {
public:
    //
    // Constructor/Deconstructor.
    //
    CLGBaseTrigger(PODEntity *clEntity);
    virtual ~CLGBaseTrigger() = default;

    DefineAbstractClass( CLGBaseTrigger, CLGBasePacketEntity );

    //
    // Interface functions. 
    //
    virtual void SpawnKey(const std::string& key, const std::string& value)  override;

protected:
    virtual void InitBrushTrigger();
    virtual void InitPointTrigger();
};