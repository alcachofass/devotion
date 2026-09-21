lightningBoltDevoAlt
{
	cull none
	{
		map gfx/misc/lightning3DevoAltGold.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcmod scale  2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/lightning3DevoAltPurp.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

lightningFlashDevoAlt
{
	sort additive
	cull disable
	{
		map models/weapons2/lightning/f_lightningAlt.tga
		blendfunc GL_ONE GL_ONE
	}
}

lightningCrackleDevoAlt
{
	cull none
	
	{
		clampmap models/weaphits/electricAlt.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle .8 2 0 9
                tcMod rotate 360
	}	
        {
		clampmap models/weaphits/electricAlt.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 9.5
                tcMod rotate -202
	}	
	
}