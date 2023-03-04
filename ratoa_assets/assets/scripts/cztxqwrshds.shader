//Here be contains all weapon model shaders.
//machinegun
models/weapons2/machinegun/cztxfmg
{
	sort additive
	//cull disable
	{
		map models/weapons2/machinegun/fmg.jpg
		rgbGen wave noise 1.0 0.5 0 5 //noise was square
		blendfunc add
	}
}
//shotgun
models/weapons2/shotgun/cztxfsg
{
	sort additive
	//cull disable
	{
		map models/weapons2/shotgun/fsg.jpg
		blendfunc add
	}
}
//rocket launcher
models/weapons2/rocketl/cztxfrl
{
	sort additive
	//cull disable
	{
		map models/weapons2/rocketl/frl.tga
		blendfunc add
		tcMod rotate -9
	}
}
//plasma gun
models/weapons2/plasma/cztxfpg
{
	//cull disable
	sort additive
	{
		map models/weapons2/plasma/fpg1.jpg
		rgbGen wave inversesawtooth 0.0 1.0 0 5
		tcMod rotate 9
		blendfunc add
	}
}

models/weapons2/plasma/cztxpg_det
{
	{
		map models/weapons2/plasma/cztxpg_glw.tga
		tcMod scroll -1.0 4.0
	}
	{
		map models/weapons2/plasma/cztxpg_det.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
}
models/weapons2/plasma/cztxpg_acc
{
	{
		map models/weapons2/plasma/cztxpg_glw.tga
		tcMod scroll -1.0 4.0
	}
	{
		map models/weapons2/plasma/cztxpg_acc.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
}
models/weapons2/plasma/cztxpg_bod
{
	{
		map models/weapons2/plasma/cztxpg_glw.tga
		tcMod scroll -1.0 4.0
	}
	{
		map models/weapons2/plasma/cztxpg_bod.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
}
//railgun
models/weapons2/railgun/cztxfrg
{
	//cull disable
	sort additive
	{
		map models/weapons2/railgun/frg.tga
		tcMod rotate 420
		blendfunc add
	}
}
models/weapons2/railgun/cztxrggw
{
	{
		map models/weapons2/railgun/cztxrggw3.tga
		tcMod scroll 2.0 4.0
		//rgbGen entity
	}
	{
		map models/weapons2/railgun/cztxrggw2.tga
		tcMod scroll 0.0 -0.1
		blendfunc filter
		//rgbGen entity
	}
	{
		map models/weapons2/railgun/cztxrggw1.tga
		blendfunc blend
		//rgbGen lightingDiffuse
	}
}
//lightning gun
models/weapons2/lightning/flg
{
	{
		animmap 20 models/weapons2/lightning/flg1.jpg models/weapons2/lightning/flg2.jpg
		rgbGen wave noise 1.0 0.8 0 30
		blendfunc add
	}
}
models/weapons2/lightning/cztxltghb
{
	{
		map models/weapons2/lightning/cztxltghb1.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/lightning/cztxltghb2.tga
		rgbGen wave noise 0.5 1.0 0 13
		blendfunc add
	}
}
models/weapons2/lightning/cztxltgcr
{
	{
		map models/weapons2/lightning/cztxltgcr1.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/lightning/cztxltgcr2.tga
		rgbGen wave noise 0.6 1.0 0 15
		blendfunc add
	}
}
models/weapons2/lightning/cztxltgdt
{
	{
		map models/weapons2/lightning/lgg1.tga
		tcMod scroll 0.05 0.2
		tcmod scale 2.0 2.0
	}
	{
		map models/weapons2/lightning/lgg2.tga
		//rgbGen wave noise 0.95 1.0 0 4
		tcMod scroll -4.0 -2.0
		tcmod scale 2.0 2.0
		blendfunc add
	}
	{
		map models/weapons2/lightning/cztxltgdt1.tga
		blendfunc blend
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/lightning/cztxltgdt2.tga
		rgbGen wave noise 0.75 1.0 0 14
		blendfunc add
	}
}