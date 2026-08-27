models/powerups/keys/key_master
{
	{
		map textures/sfx/portalfog.tga
		rgbGen identity
	}
	{
		map textures/sfx/mirror.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave sin 0.5 0.5 0 0.4
		tcMod rotate 30
		tcMod scroll 0.1 0.2
	}
}
models/powerups/keys/key_silver
{
	{
		map textures/sfx/portalfog.tga
		rgbGen identity
	}
	{
		map textures/effects/tinfx.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave sin 0.5 0.5 0 0.4
		tcMod rotate 30
		tcMod scroll 0.1 0.2
	}
}
models/powerups/keys/key_golden
{
	{
		map textures/sfx/portalfog.tga
		rgbGen identity
	}
	{
		map textures/effects/envmapgold.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen wave sin 0.5 0.5 0 0.4
		tcMod rotate 30
		tcMod scroll 0.1 0.2
	}
}
//outer wrap texture around key
models/powerups/keys/key_master_snake
{
        q3map_surfacelight	1000
        surfaceparm	trans
	surfaceparm nomarks
	surfaceparm nolightmap
	qer_editorimage textures/sfx/zap_scroll.tga
	cull none
	
	{
		map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle .8 2 0 7
                tcMod scroll 0 1
	}	
        {
		map textures/sfx/zap_scroll.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 5
                tcMod scale  -1 1
                tcMod scroll 0 1
	}	
        {
		map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 6.3
                tcMod scale  -1 1
                tcMod scroll 2 1
	}	
        {
		map textures/sfx/zap_scroll2.tga
		blendFunc GL_ONE GL_ONE
                rgbgen wave triangle 1 1.4 0 7.7
                tcMod scroll -1.3 1
	}	
}

// HUD / cg_simpleItems sprites. alphaGen oneMinusEntity is required so
// CG_Item can draw them in-world with entity alpha 0 (full opacity).
icons/key_silver
{
	nopicmip
	{
		map icons/key_silver.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen oneMinusEntity
	}
}

icons/key_gold
{
	nopicmip
	{
		map icons/key_gold.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen oneMinusEntity
	}
}

icons/key_master
{
	nopicmip
	{
		map icons/key_master.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen oneMinusEntity
	}
}