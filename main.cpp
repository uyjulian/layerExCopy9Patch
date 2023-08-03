/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2023-2023 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "ncbind/ncbind.hpp"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility.h"
#include "ComplexRect.h"
#include "ResampleImage.h"
#include "RectItf.h"
#include <string.h>
#include <stdio.h>

void Read9PatchInfo( const tTVPBaseBitmap *refbmp, tTVPRect& scale, tTVPRect& margin )
{
	const tjs_int w = refbmp->GetWidth();
	const tjs_int h = refbmp->GetHeight();
	const tjs_uint32 *src = (const tjs_uint32*)refbmp->GetScanLine(0);
	const tjs_int pitch = refbmp->GetPitchBytes() / sizeof(tjs_uint32);
	const tjs_uint32 *srcbottom = (const tjs_uint32*)refbmp->GetScanLine(h-1);
	scale = tTVPRect(-1,-1,-1,-1);
	margin = tTVPRect( -1, -1, -1, -1 );
	for( tjs_int x = 1; x < (w-1); x++ )
	{
		if( scale.left == -1 && (src[x]&0xff000000) == 0xff000000 )
		{
			scale.left = x;
		}
		else if( scale.left != -1 && scale.right == -1 && (src[x]&0xff000000) == 0 )
		{
			scale.right = x;
		}

		if( margin.left == -1 && (srcbottom[x]&0xff000000) == 0xff000000 )
		{
			margin.left = x-1;
		}
		else if( margin.left != -1 && margin.right == -1 && (srcbottom[x]&0xff000000) == 0 )
		{
			margin.right = w-x-1;
		}

		if( scale.right != -1 && margin.right != -1 ) break;
	}
	if( scale.left != -1 && scale.right == -1 ) {
		scale.right = w - 1;
	}
	if( margin.left != -1 && margin.right == -1 ) {
		margin.right = 0;
	}

	for( tjs_int y = 1; y < (h-1); y++ )
	{
		if( scale.top == -1 && (src[y*pitch]&0xff000000) == 0xff000000 )
		{
			scale.top = y;
		}
		else if( scale.top != -1 && scale.bottom == -1 && (src[y*pitch]&0xff000000) == 0 )
		{
			scale.bottom = y;
		}

		if( margin.top == -1 && (src[y*pitch+w-1]&0xff000000) == 0xff000000 )
		{
			margin.top = y-1;
		}
		else if( margin.top != -1 && margin.bottom == -1 && (src[y*pitch+w-1]&0xff000000) == 0 )
		{
			margin.bottom = h-y-1;
		}

		if( scale.bottom != -1 && margin.bottom != -1 ) break;
	}
	if( scale.top != -1 && scale.bottom == -1 ) {
		scale.bottom = h - 1;
	}
	if( margin.top != -1 && margin.bottom == -1 ) {
		margin.bottom = 0;
	}
}
//---------------------------------------------------------------------------
bool Copy9Patch( tTVPBaseBitmap *destbmp, const tTVPBaseBitmap *ref, tTVPRect& margin )
{
#if 0
	if(!Is32BPP()) return false;
#endif

	tjs_int w = ref->GetWidth();
	tjs_int h = ref->GetHeight();
	// 9 + 上下の11ピクセルは必要
	if( w < 11 || h < 11 ) return false;
	tjs_int dw = destbmp->GetWidth();
	tjs_int dh = destbmp->GetHeight();
	// コピー先が元画像よりも小さい時はコピー不可
	if( dw < (w-2) || dh < (h-2) ) return false;

	tTVPRect scale( -1, -1, -1, -1 );
	Read9PatchInfo( ref, scale, margin );

	// スケール用の領域が見付からない時はコピーできない
	if( scale.left == -1 || scale.right == -1 || scale.top == -1 || scale.bottom == -1 )
		return false;

	const tjs_uint32 *src = (const tjs_uint32*)ref->GetScanLine( 0 );
	const tjs_int pitch = ref->GetPitchBytes() / sizeof( tjs_uint32 );

	const tjs_int src_left_width = scale.left - 1;
	const tjs_int src_right_width = w - 1 - scale.right;
	const tjs_int dst_center_width = dw - src_left_width - src_right_width;
	const tjs_int src_center_width = scale.right - scale.left;
	const tjs_int src_top_height = scale.top - 1;
	const tjs_int src_bottom_height = h - 1 - scale.bottom;
	const tjs_int src_center_height = scale.bottom - scale.top;
	const tjs_int dst_center_height = dh - src_top_height - src_bottom_height;
	const tjs_int src_center_step = (src_center_width<<16) / dst_center_width;

	tjs_uint32 *dst = (tjs_uint32*)destbmp->GetScanLineForWrite(0);
	tjs_int dpitch = destbmp->GetPitchBytes() / sizeof(tjs_uint32);
	const tjs_uint32 *s1 = src + pitch + 1;
	const tjs_uint32 *s2 = src + pitch + scale.right;
	tjs_uint32 *d1 = dst;
	tjs_uint32 *d2 = dst + src_left_width + dst_center_width;
	// 上側左右端のコピー
	for( tjs_int y = 0; y < src_top_height; y++ )
	{
		memcpy( d1, s1, src_left_width*sizeof(tjs_uint32));
		memcpy( d2, s2, src_right_width*sizeof(tjs_uint32));
		d1 += dpitch; s1 += pitch;
		d2 += dpitch; s2 += pitch;
	}
	// 上側中間
	const tjs_uint32 *s3 = src + pitch + scale.left;
	tjs_uint32 *d3 = dst + src_left_width;
	if( src_center_width == 1 )
	{   // コピー元の幅が1の時はその色で塗りつぶす
		for( tjs_int y = 0; y < src_top_height; y++ )
		{
			TVPFillARGB( d3, dst_center_width, *s3 );
			d3 += dpitch; s3 += pitch;
		}
	}
	//else if( v.r == 0 ) { /* scale */ } else if( v.r == 255 ) { /* repeate */ } else { /* mirror */}
	else
	{   // scale
		for( tjs_int y = 0; y < src_top_height; y++ )
		{   // 縦方向はブレンドしないので高速化出来るが……
			TVPInterpStretchCopy( d3, dst_center_width, s3, s3, 0, 0, src_center_step );
			d3 += dpitch; s3 += pitch;
		}
	}
	// 中間位置
	// s1 s2 s3 d1 d2 d3 は、中間位置を指しているはず
	if( src_center_height == 1 )
	{
		// 中間位置の両端
		for( tjs_int y = 0; y < dst_center_height; y ++ )
		{
			memcpy( d1, s1, src_left_width*sizeof(tjs_uint32));
			memcpy( d2, s2, src_right_width*sizeof(tjs_uint32));
			d1 += dpitch; d2 += dpitch;
		}
		// 中間位置の真ん中
		if( src_center_width == 1 )
		{
			for( tjs_int y = 0; y < dst_center_height; y ++ )
			{
				TVPFillARGB( d3, dst_center_width, *s3 );
				d3 += dpitch;
			}
		}
		else
		{
			for( tjs_int y = 0; y < dst_center_height; y ++ )
			{
				TVPInterpStretchCopy( d3, dst_center_width, s3, s3, 0, 0, src_center_step );
				d3 += dpitch;
			}
		}
	}
	else
	{
		tTVPRect cliprect(0,0,dw,dh);
		{		// 左側
			tTVPRect srcrect( 1,        scale.top,     scale.left, scale.bottom );
			tTVPRect dstrect( 0,   src_top_height, src_left_width,   (src_top_height+dst_center_height) );
			TVPResampleImage( cliprect, destbmp, dstrect, ref, srcrect, stSemiFastLinear, 0.0f, bmCopy, 255, false );
		}
		{		// 中間
			tTVPRect srcrect(     scale.left,      scale.top,                     scale.right, scale.bottom );
			tTVPRect dstrect( src_left_width, src_top_height, src_left_width+dst_center_width, src_top_height+dst_center_height );
			TVPResampleImage( cliprect, destbmp, dstrect, ref, srcrect, stSemiFastLinear, 0.0f, bmCopy, 255, false );   
		}
		{		// 右側
			tTVPRect srcrect(          scale.right,        scale.top, w-1, scale.bottom );
			tTVPRect dstrect( dw - src_right_width,   src_top_height,  dw,   src_top_height+dst_center_height );
			TVPResampleImage( cliprect, destbmp, dstrect, ref, srcrect, stSemiFastLinear, 0.0f, bmCopy, 255, false );
		}
	}
	
	// 下側左右端のコピー
	s1 = src + pitch * scale.bottom + 1;
	s2 = src + pitch * scale.bottom + scale.right;
	d1 = dst + dpitch * (dh-src_bottom_height);
	d2 = dst + dpitch * (dh-src_bottom_height) + src_left_width + dst_center_width;
	for( tjs_int y = 0; y < src_bottom_height; y++ )
	{
		memcpy( d1, s1, src_left_width*sizeof(tjs_uint32));
		memcpy( d2, s2, src_right_width*sizeof(tjs_uint32));
		d1 += dpitch; s1 += pitch;
		d2 += dpitch; s2 += pitch;
	}
	// 下側中間
	s3 = src + pitch * scale.bottom + scale.left;
	d3 = dst + dpitch * (dh-src_bottom_height) + src_left_width;
	if( src_center_width == 1 )
	{   // コピー元の幅が1の時はその色で塗りつぶす
		for( tjs_int y = 0; y < src_bottom_height; y++ )
		{
			TVPFillARGB( d3, dst_center_width, *s3 );
			d3 += dpitch; s3 += pitch;
		}
	}
	else
	{   // scale
		for( tjs_int y = 0; y < src_bottom_height; y++ )
		{
			TVPInterpStretchCopy( d3, dst_center_width, s3, s3, 0, 0, src_center_step );
			d3 += dpitch; s3 += pitch;
		}
	}
	return true;
}
//---------------------------------------------------------------------------

static void PreRegistCallback()
{
	TVPCreateTable();
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object)
		{
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("copy9Patch"), 0, NULL);
		}
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);

class LayerLayerExCopy9Patch
{
public:
	static tjs_error TJS_INTF_METHOD copy9Patch(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if (numparams < 2)
		{
			return TJS_E_BADPARAMCOUNT;
		}

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();
		tTJSVariantClosure srcbmpobject_clo = param[0]->AsObjectClosureNoAddRef();

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPRect margin;
		bool updated = Copy9Patch(&bmpinfo, &srcbmpinfo, margin);
		if (result)
		{
			if (updated)
			{
				iTJSDispatch2 *ret = TVPCreateRectObject(margin.left, margin.top, margin.right, margin.bottom);
				*result = tTJSVariant(ret, ret);
				ret->Release();
			}
			else
			{
				result->Clear();
			}
		}
		if (updated)
		{
			UpdateWholeLayerWithLayerObject(bmpobject_clo);
		}
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(LayerLayerExCopy9Patch, Layer)
{
	RawCallback("copy9Patch", &Class::copy9Patch, 0);
};
