# VScript Functions
## What is this?
This is a [SourceMod](http://www.sourcemod.net/) extension that exposes [VScript](https://developer.valvesoftware.com/wiki/VScript) functions and methods to be used by modders.
## How to build this?
Just as any other [AMBuild](https://wiki.alliedmods.net/AMBuild) project:
1. [Install AMBuild](https://wiki.alliedmods.net/AMBuild#Installation)
2. Download [Half-Life 2 SDK](https://github.com/alliedmodders/hl2sdk), [Metamod:Source](https://github.com/alliedmodders/metamod-source/) and [SourceMod](https://github.com/alliedmodders/sourcemod)
3. `mkdir build && cd build`
4. `python ../configure.py --hl2sdk-root=??? --mms-path=??? --sm-path=??? --sdks=csgo`
5. `ambuild`
## How to use this?
You need to either load this module by creating `vscriptfun.autoload` file in the extensions folder or use the metamod menu to perform a manual load.
After a map is loaded, two files are created:
1. `data/vscriptfun/natives` - a list of natives that will be registered
2. `scripting/include/vscriptfun.inc` - a sourcepawn include file, containing all the interesting classes
Now you can simply create a plugin that uses VScript functions.
## Sample script
```pawn
#include <sourcemod>
#include <vscriptfun>

public Plugin myinfo =
{
	name = "My First Plugin",
	author = "Me",
	description = "My first plugin ever",
	version = "1.0",
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	RegAdminCmd("vsf_slap", Slap, ADMFLAG_GENERIC);
}

public Action Slap(int client, int args)
{
	if(!client)
		return Plugin_Handled;
	VSF_CBaseEntity player = VSF_CBaseEntity.FromEntIndex(client);
	
	float velocity[3];
	player.GetVelocity(velocity);
	velocity[0] += VSF.RandomFloat(-512.0, 512.0);
	velocity[1] += VSF.RandomFloat(-512.0, 512.0);
	velocity[2] += VSF.RandomFloat(0.0, 512.0);
	player.SetVelocity(velocity);
	
	player.SetHealth(player.GetHealth() - 1);
	
	char sound[64];
	Format(sound, 64, "physics/flesh/flesh_impact_bullet%d.wav", VSF.RandomInt(1, 5));
	player.PrecacheScriptSound(sound);
	player.EmitSound(sound);
	
	VSF.ScriptPrintMessageChatAll("Player slapped himself with a large trout.");
	return Plugin_Handled;
}
```
## Additional information
* This was tested only in CS:GO, but any modern VScript-using Source game (like Dota 2) should be ok.
* You can use CEntities class to search and browse through entities. You can use the returned handle to construct CBaseEntity or any other derived class, but don't pass it into constructors of non-entity classes like CScriptKeyValues! (unless you want to crash your server)
* This extension used to be a part of the [Gorme](https://github.com/jonatan1024/gorme) project.

