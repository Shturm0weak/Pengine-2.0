#pragma once

namespace Pengine
{

	namespace FileFormats
	{

		inline const char* BaseMat()
		{
			return "basemat";
		}

		inline const char* Mat()
		{
			return "mat";
		}

		inline const char* Png()
		{
			return "png";
		}

		inline const char* Jpeg()
		{
			return "jpeg";
		}

		inline const char* Jpg()
		{
			return "jpg";
		}

		inline const char* Dds()
		{
			return "dds";
		}

		inline const char* Obj()
		{
			return "obj";
		}

		inline const char* Gltf()
		{
			return "gltf";
		}

		inline const char* Fbx()
		{
			return "fbx";
		}

		inline const char* Mesh()
		{
			return "mesh";
		}

		inline const char* Meshes()
		{
			return "meshes";
		}

		inline const char* Meta()
		{
			return "meta";
		}

		inline const char* Spv()
		{
			return "spv";
		}

		inline bool IsTexture(const std::string& fileFormat)
		{
			return (fileFormat == Png()
				|| fileFormat == Jpeg()
				|| fileFormat == Jpg()
				|| fileFormat == Dds());
		}

		inline bool IsMeshIntermediate(const std::string& fileFormat)
		{
			return (fileFormat == Obj()
				|| fileFormat == Gltf()
				|| fileFormat == Fbx());
		}

		inline bool IsAsset(const std::string& fileFormat)
		{
			if (IsTexture(fileFormat)
				|| fileFormat == BaseMat()
				|| fileFormat == Mat()
				|| fileFormat == Mesh())
			{
				return true;
			}

			return false;
		}
	}

}