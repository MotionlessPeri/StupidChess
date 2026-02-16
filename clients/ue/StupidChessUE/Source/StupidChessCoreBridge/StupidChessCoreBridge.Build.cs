using System.IO;
using UnrealBuildTool;

public class StupidChessCoreBridge : ModuleRules
{
	public StupidChessCoreBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		string RepoRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", ".."));
		PublicIncludePaths.Add(Path.Combine(RepoRoot, "core", "include"));
		PublicIncludePaths.Add(Path.Combine(RepoRoot, "protocol", "include"));
		PublicIncludePaths.Add(Path.Combine(RepoRoot, "server", "include"));
	}
}
