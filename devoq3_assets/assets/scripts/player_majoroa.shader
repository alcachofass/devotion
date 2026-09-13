models/players/majoroa/buh
{
	{
		map textures/effects/tinfx2.tga
		rgbGen lightingDiffuse
		tcMod rotate 5
		tcGen environment 
	}
}

models/players/majoroa/ep
{
	nopicmip
	{
		map models/players/majoroa/ep.tga
	}
	{
		map models/players/majoroa/ep.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}

models/players/majoroa/ep_h
{
	nopicmip
	{
		map models/players/majoroa/ep_h.tga
	}
	{
		map models/players/majoroa/ep_h.tga
		blendfunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen entity
	}
}
