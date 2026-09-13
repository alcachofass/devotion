models/mapobjects/flag/banner_strgg
{
	cull disable
	surfaceparm nolightmap
	surfaceparm alphashadow
	deformVertexes wave 128 sin 0 2 0 .5
	nopicmip
	sort banner
	{
		map models/mapobjects/flag/banner_strgg.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		//rgbGen identity
		rgbGen vertex
		
		// old:
		//blendfunc blend
		//rgbGen vertex
		//alphaFunc GT0
		//depthWrite
	}
}
textures/ctf/ctf_redflag
{
	surfaceparm nomarks
	cull none
	tessSize 64
	deformVertexes wave 128 sin 0 2 0 .5
	nopicmip
	{
		map textures/ctf/ctf_redflag.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
		//rgbGen vertex

		// old:
		//blendFunc blend
		//rgbGen identity
		//alphaFunc GT0
		//depthWrite
	}
}

textures/ctf/ctf_blueflag
{
	surfaceparm nomarks
	cull none
	tessSize 64
	deformVertexes wave 128 sin 0 2 0 .5
	nopicmip
	{
		map textures/ctf/ctf_blueflag.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
		//rgbGen vertex

		// old:
		//blendFunc blend
		//rgbGen identity
		//alphaFunc GT0
		//depthWrite
	}
}

// OA banners
textures/ctf2/red_banner02
{
	cull none
	nopicmip
	{
		map textures/ctf2/red_banner02.tga
		rgbgen identity
	}
}
textures/ctf2/red_banner02
{
	cull none
	nopicmip
	{
		map textures/ctf2/red_banner02.tga
		rgbgen identity
	}
}

textures/clown/blue_banner
{
	
	surfaceparm nomarks
	cull none
 	deformVertexes wave 256 sin 0 7 0 0.4
	
	{
		clampmap textures/clown/blue_banner.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity

	}
}

textures/clown/red_banner
{
	
	surfaceparm nomarks
	cull none
 	deformVertexes wave 256 sin 0 7 0 0.4
	
	{
		clampmap textures/clown/red_banner.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity

	}
}
