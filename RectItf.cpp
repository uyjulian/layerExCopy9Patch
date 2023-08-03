
#include "RectItf.h"
iTJSDispatch2 * TVPCreateRectObject( tjs_int left, tjs_int top, tjs_int right, tjs_int bottom )
{
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() {
			iTJSDispatch2 *global = TVPGetScriptDispatch();
			tTJSVariant val;
			global->PropGet(0, TJS_W("Rect"), 0, &val, global);
			if (val.Type() == tvtObject)
			{
				Obj = val.AsObject();
			}
			else
			{
				global->PropGet(0, TJS_W("Dictionary"), 0, &val, global);
				Obj = val.AsObject();
			}
		}
		~tHolder() { Obj->Release(); }
	} static rectclass;

	iTJSDispatch2 *out;
	tjs_error hr = rectclass.Obj->CreateNew(0, NULL, NULL, &out, 0, NULL, rectclass.Obj);
	if(TJS_FAILED(hr)) TVPThrowExceptionMessage(TJS_W("Could not create Rect object"));
	if (out)
	{
		tTJSVariant val;
		val = left;
		out->PropSet(TJS_MEMBERENSURE, TJS_W("left"), NULL, &val, out);
		val = top;
		out->PropSet(TJS_MEMBERENSURE, TJS_W("top"), NULL, &val, out);
		val = right;
		out->PropSet(TJS_MEMBERENSURE, TJS_W("right"), NULL, &val, out);
		val = bottom;
		out->PropSet(TJS_MEMBERENSURE, TJS_W("bottom"), NULL, &val, out);
	}

	return out;
}
