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

railAltExplosion
{
	cull disable
        {
		animmap 5 models/weaphits/ring02_1.tga  models/weaphits/ring02_2.tga  models/weaphits/ring02_3.tga models/weaphits/ring02_4.tga models/weaphits/ring02_3.tga models/weaphits/ring02_2.tga models/weaphits/ring02_1.tga gfx/colors/black.tga
		alphaGen wave inversesawtooth 0 1 0 8
		rgbGen const ( 0.95 0.75 0.2 )
		blendfunc blend
	}
	{
		animmap 5 models/weaphits/ring02_2.tga  models/weaphits/ring02_3.tga  models/weaphits/ring02_4.tga models/weaphits/ring02_3.tga models/weaphits/ring02_2.tga models/weaphits/ring02_1.tga gfx/colors/black.tga gfx/colors/black.tga
		alphaGen wave sawtooth 0 1 0 8
		rgbGen const ( 0.8 0.3 1.0 )
		blendfunc blend
	}
}

vortexShell
{
	//deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/vortexPurp.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
                tcmod rotate 30
		        tcmod scroll 1 .1
	}
	{
		map textures/effects/vortexGold.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
                tcmod rotate 45
		        tcmod scroll 1 .15
	}
}