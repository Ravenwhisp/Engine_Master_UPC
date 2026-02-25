#include "Globals.h"
#include "Component.h"
#include "Frustum.h"

class CameraComponent : public Component 
{
public:
	CameraComponent(UID id, GameObject* gameObject);

	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) override;
	void update() override;

	const float getFov() const { return m_horizontalFov; }
	void setFov(float fov) { m_horizontalFov = fov; }

	const float getNearPlane() const { return m_nearPlane; }
	void setNearPlane(float nearPlane) { m_nearPlane = nearPlane; }

	const float getFarPlane() const { return m_farPlane; }
	void setFarPlane(float farPlane) { m_farPlane = farPlane; }

	const Matrix& getWorldMatrix() const { return m_world; }

	const Matrix& getViewMatrix() const { return m_view; }

	const Matrix& getProjectionMatrix() const { return m_projection; }

	const Engine::Frustum& getFrustum() const { return m_frustum; }

	void recalculateFrustum();

	void drawUi() override;

	void onTransformChange() override;

	bool cleanUp() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;


private:
	float m_horizontalFov = 90.0f;
	float m_nearPlane = 0.5f;
	float m_farPlane = 25.0f;
	float m_aspectRatio = 16.0f / 9.0f;

	Matrix m_world = {};
	Matrix m_view = {};
	Matrix m_projection = {};

	Engine::Frustum m_frustum = {};
};