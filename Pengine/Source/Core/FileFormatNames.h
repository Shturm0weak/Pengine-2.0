#pragma once

namespace Pengine::FileFormats
{
	inline const char* BaseMat()
	{
		return ".basemat";
	}

	inline const char* Mat()
	{
		return ".mat";
	}

	inline const char* Png()
	{
		return ".png";
	}

	inline const char* Jpeg()
	{
		return ".jpeg";
	}

	inline const char* Jpg()
	{
		return ".jpg";
	}

	inline const char* Dds()
	{
		return ".dds";
	}

	inline const char* Tga()
	{
		return ".tga";
	}

	inline const char* Obj()
	{
		return ".obj";
	}

	inline const char* Gltf()
	{
		return ".gltf";
	}

	inline const char* Fbx()
	{
		return ".fbx";
	}

	inline const char* Mesh()
	{
		return ".mesh";
	}

	inline const char* Meta()
	{
		return ".meta";
	}

	inline const char* Spv()
	{
		return ".spv";
	}

	inline const char* Prefab()
	{
		return ".prefab";
	}

	inline const char* Scene()
	{
		return ".scene";
	}

	inline bool IsTexture(const std::string& fileFormat)
	{
		return fileFormat == Png()
			|| fileFormat == Jpeg()
			|| fileFormat == Jpg()
			|| fileFormat == Dds()
			|| fileFormat == Tga();
	}

	inline bool IsMeshIntermediate(const std::string& fileFormat)
	{
		return fileFormat == Obj()
			|| fileFormat == Gltf()
			|| fileFormat == Fbx();
	}

	inline bool IsAsset(const std::string& fileFormat)
	{
		if (IsTexture(fileFormat)
			|| fileFormat == BaseMat()
			|| fileFormat == Mat()
			|| fileFormat == Mesh()
			|| fileFormat == Prefab())
		{
			return true;
		}

		return false;
	}

}