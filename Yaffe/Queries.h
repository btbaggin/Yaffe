#pragma once

const char* qs_GetPlatform = "SELECT Path, Args, Roms FROM Platforms WHERE ID = @ID";
const char* qs_GetAllPlatforms = "SELECT ID, Platform FROM Platforms";
const char* qs_AddPlatform = "INSERT INTO Platforms ( ID, Platform, Path, Args, Roms ) VALUES ( @PlatformId, @Platform, @Path, @Args, @Roms )";

const char* qs_GetGame = "SELECT ID, Name, Overview, Players, FileName FROM Games WHERE Platform = @Platform AND FileName = @Game";
const char* qs_AddGame = "INSERT INTO Games (ID, Platform, Name, Overview, Players, FileName) VALUES ( @GameId, @Platform, @Name, @Overview, @Players, @FileName )";

const char* qs_GetAllApplications = "SELECT Name, Path, Args FROM Applications";
const char* qs_AddApplication = "INSERT INTO Applications ( Name, Path, Args ) VALUES ( @Name, @Path, @Args )";