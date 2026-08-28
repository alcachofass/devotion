ratmod_menulogo_white
{
	nopicmip
	nomipmaps
	{
		map textures/sfx/devotion_logo_white.tga
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}

menubacknologo_ratmod
{
	
	nopicmip
	nomipmaps

	{
		map $whiteimage 
		rgbGen const ( 0.00 0.00 0.00 ) //0.03 0.03 0.03

	}
}

menuback_ratmod
{
	nopicmip
	nomipmaps
	{
		map textures/liquids/lavahell
		rgbGen const ( 0.45 0.12 1.0 )
		tcMod scroll 0.05 0.05
	}
	{
		map textures/liquids/lavahell
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.20 0.02 0.12 )
		tcMod scroll -0.02 0.05
		tcMod scale -1.1 0.8
	}
	{
		map textures/sfx/detail.jpg
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbGen const ( 0.55 0.55 0.55 )
		tcMod scale 2 2
	}
}
