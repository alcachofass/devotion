ratmod_menulogo_white
{
	nopicmip
	{
		map textures/sfx/devotion_logo_white.tga
		rgbGen const ( 1 0 0 )
		blendFunc blend
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
	{
		map textures/liquids/lavahell
		rgbGen const ( 0.1 0 0.8 )
		tcMod scroll 0.05 0.05
	}
	{
		map textures/liquids/lavahell
		blendfunc gl_dst_color gl_src_color
		rgbGen const ( 0.33 0 0 )
		tcMod scroll -0.02 0.05
		tcMod scale -1.1 0.8
	}
	{
		map textures/sfx/detail.jpg
		blendfunc gl_dst_color gl_src_color
		tcMod scale 2 2
	}
}
