#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "light.h"
#include "lc_application.h"
#include "lc_context.h"

#define LC_LIGHT_POSITION_EDGE 7.5f
#define LC_LIGHT_TARGET_EDGE 5.0f
#define LC_LIGHT_SPHERE_RADIUS 5.0f

// New omni light.
lcLight::lcLight(float px, float py, float pz)
	: lcObject(lcObjectType::Light)
{
	Initialize(lcVector3(px, py, pz), lcVector3(0.0f, 0.0f, 0.0f));
	UpdatePosition(1);
}

// New directional or spot light.
lcLight::lcLight(float px, float py, float pz, float tx, float ty, float tz)
	: lcObject(lcObjectType::Light)
{
	Initialize(lcVector3(px, py, pz), lcVector3(tx, ty, tz));
	mState |= LC_LIGHT_SPOT;
	UpdatePosition(1);
}

void lcLight::Initialize(const lcVector3& Position, const lcVector3& TargetPosition)
{
	mState = 0;

	mPositionKeys.ChangeKey(Position, 1, true);
	mTargetPositionKeys.ChangeKey(TargetPosition, 1, true);
	mAmbientColorKeys.ChangeKey(lcVector4(0.0f, 0.0f, 0.0f, 1.0f), 1, true);
	mDiffuseColorKeys.ChangeKey(lcVector4(0.8f, 0.8f, 0.8f, 1.0f), 1, true);
	mSpecularColorKeys.ChangeKey(lcVector4(1.0f, 1.0f, 1.0f, 1.0f), 1, true);
	mAttenuationKeys.ChangeKey(lcVector3(1.0f, 0.0f, 0.0f), 1, true);
	mSpotCutoffKeys.ChangeKey(30.0f, 1, true);
	mSpotExponentKeys.ChangeKey(0.0f, 1, true);
}

lcLight::~lcLight()
{
}

void lcLight::SaveLDraw(QTextStream& Stream) const
{
	Q_UNUSED(Stream);
}

void lcLight::CreateName(const lcArray<lcLight*>& Lights)
{
	if (!mName.isEmpty())
	{
		bool Found = false;

		for (const lcLight* Light : Lights)
		{
			if (Light->GetName() == mName)
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int MaxLightNumber = 0;
	const QLatin1String Prefix("Light ");

	for (const lcLight* Light : Lights)
	{
		QString LightName = Light->GetName();

		if (LightName.startsWith(Prefix))
		{
			bool Ok = false;
			int LightNumber = LightName.mid(Prefix.size()).toInt(&Ok);

			if (Ok && LightNumber > MaxLightNumber)
				MaxLightNumber = LightNumber;
		}
	}

	mName = Prefix + QString::number(MaxLightNumber + 1);
}

void lcLight::CompareBoundingBox(lcVector3& Min, lcVector3& Max)
{
	const lcVector3 Points[2] =
	{
		mPosition, mTargetPosition
	};

	for (int i = 0; i < (IsPointLight() ? 1 : 2); i++)
	{
		const lcVector3& Point = Points[i];

		// TODO: this should check the entire mesh

		Min = lcMin(Point, Min);
		Max = lcMax(Point, Max);
	}
}

void lcLight::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	if (IsPointLight())
	{
		float Distance;

		if (lcSphereRayMinIntersectDistance(mPosition, LC_LIGHT_SPHERE_RADIUS, ObjectRayTest.Start, ObjectRayTest.End, &Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}

		return;
	}

	lcVector3 Min = lcVector3(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE);
	lcVector3 Max = lcVector3(LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldLight);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldLight);

	float Distance;
	lcVector3 Plane;

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}

	Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	Max = lcVector3(LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	Start = lcMul31(ObjectRayTest.Start, WorldTarget);
	End = lcMul31(ObjectRayTest.End, WorldTarget);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}
}

void lcLight::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mPosition, ObjectBoxTest.Planes[PlaneIdx]) + ObjectBoxTest.Planes[PlaneIdx][3] > LC_LIGHT_SPHERE_RADIUS)
				return;

		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	lcVector3 Min(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE);
	lcVector3 Max(LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldLight);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldLight[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	Max = lcVector3(LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldTarget);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldTarget[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}
}

void lcLight::MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		mPosition += Distance;
		mPositionKeys.ChangeKey(mPosition, Step, AddKey);
	}

	if (IsSelected(LC_LIGHT_SECTION_TARGET))
	{
		mTargetPosition += Distance;
		mTargetPositionKeys.ChangeKey(mTargetPosition, Step, AddKey);
	}
}

void lcLight::InsertTime(lcStep Start, lcStep Time)
{
	mPositionKeys.InsertTime(Start, Time);
	mTargetPositionKeys.InsertTime(Start, Time);
	mAmbientColorKeys.InsertTime(Start, Time);
	mDiffuseColorKeys.InsertTime(Start, Time);
	mSpecularColorKeys.InsertTime(Start, Time);
	mAttenuationKeys.InsertTime(Start, Time);
	mSpotCutoffKeys.InsertTime(Start, Time);
	mSpotExponentKeys.InsertTime(Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	mPositionKeys.RemoveTime(Start, Time);
	mTargetPositionKeys.RemoveTime(Start, Time);
	mAmbientColorKeys.RemoveTime(Start, Time);
	mDiffuseColorKeys.RemoveTime(Start, Time);
	mSpecularColorKeys.RemoveTime(Start, Time);
	mAttenuationKeys.RemoveTime(Start, Time);
	mSpotCutoffKeys.RemoveTime(Start, Time);
	mSpotExponentKeys.RemoveTime(Start, Time);
}

void lcLight::UpdatePosition(lcStep Step)
{
	mPosition = mPositionKeys.CalculateKey(Step);
	mTargetPosition = mTargetPositionKeys.CalculateKey(Step);
	mAmbientColor = mAmbientColorKeys.CalculateKey(Step);
	mDiffuseColor = mDiffuseColorKeys.CalculateKey(Step);
	mSpecularColor = mSpecularColorKeys.CalculateKey(Step);
	mAttenuation = mAttenuationKeys.CalculateKey(Step);
	mSpotCutoff = mSpotCutoffKeys.CalculateKey(Step);
	mSpotExponent = mSpotExponentKeys.CalculateKey(Step);

	if (IsPointLight())
	{
		mWorldLight = lcMatrix44Identity();
		mWorldLight.SetTranslation(-mPosition);
	}
	else
	{
		lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 UpVector(1, 1, 1);

		if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
		{
			if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
				UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}
		else
		{
			if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
				UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}

		mWorldLight = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	}
}

void lcLight::DrawInterface(lcContext* Context, const lcScene& Scene) const
{
	Q_UNUSED(Scene);
	Context->SetMaterial(lcMaterialType::UnlitColor);

	if (IsPointLight())
		DrawPointLight(Context);
	else
		DrawSpotLight(Context);
}

void lcLight::DrawSpotLight(lcContext* Context) const
{
	lcVector3 FrontVector(mTargetPosition - mPosition);
	lcVector3 UpVector(1, 1, 1);

	if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
	{
		if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
			UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
		else
			UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
	}
	else
	{
		if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
			UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
		else
			UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
	}

	lcMatrix44 LightMatrix = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	LightMatrix = lcMatrix44AffineInverse(LightMatrix);
	LightMatrix.SetTranslation(lcVector3(0, 0, 0));

	const lcMatrix44 LightViewMatrix = lcMul(LightMatrix, lcMatrix44Translation(mPosition));
	Context->SetWorldMatrix(LightViewMatrix);

	float Verts[(20 + 8 + 2 + 16) * 3];
	float* CurVert = Verts;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		float c = cosf((float)EdgeIdx / 4 * LC_PI) * LC_LIGHT_POSITION_EDGE;
		float s = sinf((float)EdgeIdx / 4 * LC_PI) * LC_LIGHT_POSITION_EDGE;

		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = LC_LIGHT_POSITION_EDGE;
		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = -LC_LIGHT_POSITION_EDGE;
	}

	*CurVert++ = -12.5f; *CurVert++ = -12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ =  12.5f; *CurVert++ = -12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ =  12.5f; *CurVert++ =  12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ = -12.5f; *CurVert++ =  12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;

	float Length = FrontVector.Length();

	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -Length;

	const GLushort Indices[56 + 24 + 2 + 40] = 
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 0,
		1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15, 1,
		16, 17, 17, 18, 18, 19, 19, 16,
		20, 21, 21, 22, 22, 23, 23, 20,
		24, 25, 25, 26, 26, 27, 27, 24,
		20, 24, 21, 25, 22, 26, 23, 27,
		28, 29,
		30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38,
		38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 30,
		28, 30, 28, 34, 28, 38, 28, 42
	};

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(Indices);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;
	const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
	const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
	const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);

	if (!IsSelected())
	{
		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);

		Context->DrawIndexedPrimitives(GL_LINES, 56 + 24 + 2, GL_UNSIGNED_SHORT, 0);
	}
	else
	{
		if (IsSelected(LC_LIGHT_SECTION_POSITION))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_POSITION))
				Context->SetColor(FocusedColor);
			else
				Context->SetColor(SelectedColor);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetColor(LightColor);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 56, GL_UNSIGNED_SHORT, 0);

		if (IsSelected(LC_LIGHT_SECTION_TARGET))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_TARGET))
				Context->SetColor(FocusedColor);
			else
				Context->SetColor(SelectedColor);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetColor(LightColor);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 56 * 2);

		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);

		float Radius = tanf(LC_DTOR * mSpotCutoff) * Length;

		for (int EdgeIdx = 0; EdgeIdx < 16; EdgeIdx++)
		{
			*CurVert++ = cosf((float)EdgeIdx / 16 * LC_2PI) * Radius;
			*CurVert++ = sinf((float)EdgeIdx / 16 * LC_2PI) * Radius;
			*CurVert++ = -Length;
		}

		Context->DrawIndexedPrimitives(GL_LINES, 2 + 40, GL_UNSIGNED_SHORT, (56 + 24) * 2);
	}
}

void lcLight::DrawPointLight(lcContext* Context) const
{
	constexpr int Slices = 6;
	constexpr int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	constexpr int NumVertices = (Slices - 1) * Slices + 2;
	constexpr float Radius = LC_LIGHT_SPHERE_RADIUS;
	lcVector3 Vertices[NumVertices];
	quint16 Indices[NumIndices];

	lcVector3* Vertex = Vertices;
	quint16* Index = Indices;

	*Vertex++ = lcVector3(0, 0, Radius);

	for (int i = 1; i < Slices; i++ )
	{
		const float r0 = Radius * sinf(i * (LC_PI / Slices));
		const float z0 = Radius * cosf(i * (LC_PI / Slices));

		for (int j = 0; j < Slices; j++)
		{
			const float x0 = r0 * sinf(j * (LC_2PI / Slices));
			const float y0 = r0 * cosf(j * (LC_2PI / Slices));

			*Vertex++ = lcVector3(x0, y0, z0);
		}
	}

	*Vertex++ = lcVector3(0, 0, -Radius);

	for (quint16 i = 0; i < Slices - 1; i++ )
	{
		*Index++ = 0;
		*Index++ = 1 + i;
		*Index++ = 1 + i + 1;
	}

	*Index++ = 0;
	*Index++ = 1;
	*Index++ = 1 + Slices - 1;

	for (quint16 i = 0; i < Slices - 2; i++ )
	{
		quint16 Row1 = 1 + i * Slices;
		quint16 Row2 = 1 + (i + 1) * Slices;

		for (quint16 j = 0; j < Slices - 1; j++ )
		{
			*Index++ = Row1 + j;
			*Index++ = Row2 + j + 1;
			*Index++ = Row2 + j;

			*Index++ = Row1 + j;
			*Index++ = Row1 + j + 1;
			*Index++ = Row2 + j + 1;
		}

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row2 + Slices - 1;

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row1 + 0;
	}

	for (quint16 i = 0; i < Slices - 1; i++ )
	{
		*Index++ = (Slices - 1) * Slices + 1;
		*Index++ = (Slices - 1) * (Slices - 1) + i;
		*Index++ = (Slices - 1) * (Slices - 1) + i + 1;
	}

	*Index++ = (Slices - 1) * Slices + 1;
	*Index++ = (Slices - 1) * (Slices - 1) + (Slices - 2) + 1;
	*Index++ = (Slices - 1) * (Slices - 1);

	Context->SetWorldMatrix(lcMatrix44Translation(mPosition));

	const lcPreferences& Preferences = lcGetPreferences();

	if (IsFocused(LC_LIGHT_SECTION_POSITION))
	{
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
		Context->SetColor(FocusedColor);
	}
	else if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		Context->SetColor(SelectedColor);
	}
	else
	{
		const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);
		Context->SetColor(LightColor);
	}

	Context->SetVertexBufferPointer(Vertices);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(Indices);
	Context->DrawIndexedPrimitives(GL_TRIANGLES, NumIndices, GL_UNSIGNED_SHORT, 0);
}

void lcLight::RemoveKeyFrames()
{
	mPositionKeys.RemoveAll();
	mPositionKeys.ChangeKey(mPosition, 1, true);

	mTargetPositionKeys.RemoveAll();
	mTargetPositionKeys.ChangeKey(mTargetPosition, 1, true);

	mAmbientColorKeys.RemoveAll();
	mAmbientColorKeys.ChangeKey(mAmbientColor, 1, true);

	mDiffuseColorKeys.RemoveAll();
	mDiffuseColorKeys.ChangeKey(mDiffuseColor, 1, true);

	mSpecularColorKeys.RemoveAll();
	mSpecularColorKeys.ChangeKey(mSpecularColor, 1, true);

	mAttenuationKeys.RemoveAll();
	mAttenuationKeys.ChangeKey(mAttenuation, 1, true);

	mSpotCutoffKeys.RemoveAll();
	mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);

	mSpotExponentKeys.RemoveAll();
	mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);
}

bool lcLight::Setup(int LightIndex)
{
	Q_UNUSED(LightIndex);

	return true;
}
