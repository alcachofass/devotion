//
// SKELETON KEY SHADERS
// By Hipshot (http://www.zfight.com/)
// Note iron key shaders are removed, since we don't implement it.
//

models/powerups/keys/key_master_snake
{
	surfaceparm trans	
	cull none	
	{
		map models/powerups/keys/key_master_snake.tga
		blendFunc add	
		tcMod scroll -1 .5
		rgbGen wave sin 0 1 0 .5	
	}

}

models/powerups/keys/key_gold_snake
{
	surfaceparm trans	
	cull none	
	qer_trans 0.5	
	{
		map models/powerups/keys/key_gold_snake.tga
		blendFunc add	
		tcMod scroll -1 .5
		rgbGen wave sin 0 1 0 .5	
	}

}

models/powerups/keys/key_silver_snake
{
	surfaceparm trans	
	cull none	
	{
		map models/powerups/keys/key_silver_snake.tga
		blendFunc add	
		tcMod scroll -1 .5
		rgbGen wave sin 0 1 0 .5	
	}

}

models/powerups/keys/key_master
{
   {
      map models/powerups/keys/key_gold.tga
      rgbGen lightingDiffuse      
   }
   {
      map models/powerups/keys/envmap-r.tga
      blendFunc add         
      tcGen environment
      rgbGen lightingDiffuse
      tcMod scroll .05 .05
      tcmod scale 2 2
   }
   {
      map models/powerups/keys/envmap-r.tga
      blendFunc add
      tcGen environment
      tcMod scroll .05 .05
      tcmod scale 2 2
      rgbGen wave sin 0 1 0 .5
   }
   {
      map models/powerups/keys/key_master.tga
      blendFunc blend
      rgbGen lightingDiffuse
   }
}

models/powerups/keys/key_gold
{
   {
      map models/powerups/keys/key_gold.tga
      rgbGen lightingDiffuse      
   }
   {
      map models/powerups/keys/envmap-y.tga
      blendFunc add      
      tcGen environment
      rgbGen lightingDiffuse
      tcMod scroll .05 .05
      tcmod scale 2 2
   }
   {
      map models/powerups/keys/envmap-y.tga
      blendFunc add
      tcGen environment
      tcMod scroll .05 .05
      tcmod scale 2 2
      rgbGen wave sin 0 1 0 .5
   }
   {
      map models/powerups/keys/key_gold.tga
      blendFunc blend
      rgbGen lightingDiffuse
   }
}

models/powerups/keys/key_silver
{
   {
      map models/powerups/keys/key_silver.tga
      rgbGen lightingDiffuse      
   }
   {
      map models/powerups/keys/envmap-b.tga
      blendFunc add      
      tcGen environment
      rgbGen lightingDiffuse
      tcMod scroll .05 .05
      tcmod scale 2 2
   }
   {
      map models/powerups/keys/envmap-b.tga
      blendFunc add
      tcGen environment
      tcMod scroll .05 .05
      tcmod scale 2 2
      rgbGen wave sin 0 1 0 .5
   }
   {
      map models/powerups/keys/key_silver.tga
      blendFunc blend
      rgbGen lightingDiffuse
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