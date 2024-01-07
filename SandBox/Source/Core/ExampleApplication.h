#include "Core/Application.h"

#include "Core/TextureManager.h"
#include "Core/SceneManager.h"
#include "Core/MaterialManager.h"
#include "Core/ViewportManager.h"

#include "Graphics/Mesh.h"

class ExampleApplication : public Pengine::Application
{
public:

	virtual void OnPreStart() override;

	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnClose() override;
private:

};