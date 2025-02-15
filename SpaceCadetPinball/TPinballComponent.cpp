#include "pch.h"
#include "TPinballComponent.h"
#include "loader.h"
#include "objlist_class.h"
#include "render.h"
#include "TPinballTable.h"

TPinballComponent::TPinballComponent(TPinballTable* table, int groupIndex, bool loadVisuals)
{
	visualStruct visual{};

	MessageField = 0;
	UnusedBaseFlag = 0;
	ActiveFlag = 0;
	PinballTable = table;
	RenderSprite = nullptr;
	ListBitmap = nullptr;
	ListZMap = nullptr;
	GroupName = nullptr;
	Control = nullptr;
	if (table)
		table->ComponentList->Add(this);
	if (groupIndex >= 0)
		GroupName = loader::query_name(groupIndex);
	if (loadVisuals && groupIndex >= 0)
	{
		int visualCount = loader::query_visual_states(groupIndex);
		for (int index = 0; index < visualCount; ++index)
		{
			loader::query_visual(groupIndex, index, &visual);
			if (visual.Bitmap)
			{
				if (!ListBitmap)
					ListBitmap = new objlist_class<gdrv_bitmap8>(visualCount, 4);
				if (ListBitmap)
					ListBitmap->Add(visual.Bitmap);
			}
			if (visual.ZMap)
			{
				if (!ListZMap)
					ListZMap = new objlist_class<zmap_header_type>(visualCount, 4);
				if (ListZMap)
					ListZMap->Add(visual.ZMap);
			}
		}
		zmap_header_type* zMap = nullptr;
		if (ListZMap)
			zMap = ListZMap->Get(0);
		if (ListBitmap)
		{
			/* Full tilt hack - spliced bitmap includes zMap
			 * Users access bitmap-zMap in pairs, pad zMap list with 0 for such users
			 * zdrv does not access zMap when drawing spliced bitmap*/
			if (!ListZMap)
			{
				ListZMap = new objlist_class<zmap_header_type>(0, 4);
				for (int index = 0; index < ListBitmap->GetCount(); index++)
				{
					assertm(ListBitmap->Get(index)->BitmapType == BitmapTypes::Spliced, "Wrong zMap padding");
					ListZMap->Add(visual.ZMap);
				}
			}

			rectangle_type bmp1Rect{}, tmpRect{};
			auto rootBmp = ListBitmap->Get(0);
			bmp1Rect.XPosition = rootBmp->XPosition - table->XOffset;
			bmp1Rect.YPosition = rootBmp->YPosition - table->YOffset;
			bmp1Rect.Width = rootBmp->Width;
			bmp1Rect.Height = rootBmp->Height;
			for (int index = 1; index < ListBitmap->GetCount(); index++)
			{
				auto bmp = ListBitmap->Get(index);
				tmpRect.XPosition = bmp->XPosition - table->XOffset;
				tmpRect.YPosition = bmp->YPosition - table->YOffset;
				tmpRect.Width = bmp->Width;
				tmpRect.Height = bmp->Height;
				maths::enclosing_box(&bmp1Rect, &tmpRect, &bmp1Rect);
			}

			RenderSprite = render::create_sprite(
				visualCount > 0 ? VisualTypes::Sprite : VisualTypes::None,
				rootBmp,
				zMap,
				rootBmp->XPosition - table->XOffset,
				rootBmp->YPosition - table->YOffset,
				&bmp1Rect);
		}
	}
	GroupIndex = groupIndex;
}


TPinballComponent::~TPinballComponent()
{
	TPinballTable* table = PinballTable;
	if (table)
		table->ComponentList->Delete(this);

	delete ListBitmap;
	delete ListZMap;
}


int TPinballComponent::Message(int code, float value)
{
	MessageField = code;
	if (code == 1024)
		MessageField = 0;
	return 0;
}

void TPinballComponent::port_draw()
{
}

void TPinballComponent::put_scoring(int index, int score)
{
}

int TPinballComponent::get_scoring(int index)
{
	return 0;
}

void* TPinballComponent::operator new(size_t Size)
{
	return calloc(1u, Size);
}

void TPinballComponent::operator delete(void* p)
{
	free(p); /*Original does not have this*/
}
