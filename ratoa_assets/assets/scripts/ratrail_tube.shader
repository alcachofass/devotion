railTube100
{
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/railtube1.tga
		//tcMod scale 10 1

 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/railtube1_glow.tga
 		blendFunc add
 		rgbGen wave sin 0 0.5 0.25 0.6
 	}
}
// scaled up texture for shorter tubes
railTube50
{
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/railtube1.tga
		tcMod scale 0.5 1

 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/railtube1_glow.tga
		tcMod scale 0.5 1
 		blendFunc add
 		rgbGen wave sin 0 0.5 0.25 0.6
 	}
}
