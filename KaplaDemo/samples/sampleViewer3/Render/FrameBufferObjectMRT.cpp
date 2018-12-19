/**********************************************************************\
* AUTHOR : HILLAIRE S�bastien
*
* MAIL   : hillaire_sebastien@yahoo.fr
* SITE   : sebastien.hillaire.free.fr
*
*	You are free to totally or partially use this file/code.
* If you do, please credit me in your software or demo and leave this
* note.
*	Share your work and your ideas as much as possible!
\*********************************************************************/

#include "FrameBufferObjectMRT.h"


//#include "ImageBMP.h"

bool CheckFramebufferStatus(const GLuint fbo)
{
    GLenum status;
	bool ret = false;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
			//printf("Frame Buffer Created\n");
			ret = true;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			printf("Unsupported framebuffer format\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Framebuffer incomplete, missing attachment\n");
            break;
        //case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
        //    printf("Framebuffer incomplete, duplicate attachment\n");
        //   break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Framebuffer incomplete, attached images must have same dimensions\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            printf("Framebuffer incomplete, attached images must have same format\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Framebuffer incomplete, missing draw buffer\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Framebuffer incomplete, missing read buffer\n");
            break;
        default:
            printf("Framebuffer incomplete, UNKNOW ERROR\n");
            assert(0);
    }

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return ret;
}

//////////////////////////////////////////////////////////////////////////////////

FrameBufferObjectMRT::FrameBufferObjectMRT(unsigned int width, unsigned int height,
		unsigned int nbColorBuffer, 
		const unsigned int* colorBufferInternalFormat, 
		const unsigned int* colorBufferSWRAP,
		const unsigned int* colorBufferTWRAP,
		const unsigned int* colorBufferMinFiltering,
		const unsigned int* colorBufferMagFiltering,
		FBO_DepthBufferType depthBufferType,
		const unsigned int depthBufferMinFiltering,
		const unsigned int depthBufferMagFiltering,
		const unsigned int depthBufferSWRAP,
		const unsigned int depthBufferTWRAP)
{
	this->width = width;
	this->height= height;
	this->nbColorAttachement = nbColorBuffer;
	if(this->nbColorAttachement>this->getMaxColorAttachments())
		this->nbColorAttachement = this->getMaxColorAttachments();

	
	/////////////////INITIALIZATION/////////////////

	//color render buffer
	if(this->nbColorAttachement>0)
	{
		this->colorTextures = new GLuint[this->nbColorAttachement];
		this->colorMinificationFiltering = new GLuint[this->nbColorAttachement];
		for(int i=0; i<this->nbColorAttachement; i++)
		{
			glGenTextures(1, &this->colorTextures[i]);
			glBindTexture(GL_TEXTURE_2D, this->colorTextures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorBufferMinFiltering[i]);
			this->colorMinificationFiltering[i] = colorBufferMinFiltering[i];
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorBufferMagFiltering[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorBufferSWRAP[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorBufferTWRAP[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, colorBufferInternalFormat[i], width, height, 0, GL_RGB, GL_FLOAT, NULL);

			if(this->colorMinificationFiltering[i]==GL_NEAREST_MIPMAP_NEAREST
			|| this->colorMinificationFiltering[i]==GL_LINEAR_MIPMAP_NEAREST
			||this->colorMinificationFiltering[i]==GL_NEAREST_MIPMAP_LINEAR
			|| this->colorMinificationFiltering[i]==GL_LINEAR_MIPMAP_LINEAR)
			{
				glGenerateMipmapEXT(GL_TEXTURE_2D);
			}
		}
	}

	//depth render buffer
	this->depthType = depthBufferType;
	if(this->depthType!=FBO_DepthBufferType_NONE)
	{
		switch(this->depthType)
		{
		case FBO_DepthBufferType_TEXTURE:
			glGenTextures(1, &this->depthID);
			glBindTexture(GL_TEXTURE_2D, this->depthID);
			this->depthMinificationFiltering = depthBufferMinFiltering;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, depthBufferMinFiltering);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, depthBufferMagFiltering);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, depthBufferSWRAP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, depthBufferTWRAP);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

			if(this->depthMinificationFiltering==GL_NEAREST_MIPMAP_NEAREST
			|| this->depthMinificationFiltering==GL_LINEAR_MIPMAP_NEAREST
			||this->depthMinificationFiltering==GL_NEAREST_MIPMAP_LINEAR
			|| this->depthMinificationFiltering==GL_LINEAR_MIPMAP_LINEAR)
			{
				glGenerateMipmapEXT(GL_TEXTURE_2D);
			}
			break;

		case FBO_DepthBufferType_RENDERTARGET:
		default:
			glGenRenderbuffersEXT(1, &this->depthID);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, this->depthID);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24_ARB, width, height);
			break;
		}
	}

	/////////////////ATTACHEMENT/////////////////
    glGenFramebuffersEXT(1, &this->fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,this->fbo);

	//color render buffer attachement
	if(nbColorBuffer>0)
	{
		for(int i=0; i<this->nbColorAttachement; i++)
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_TEXTURE_2D, this->colorTextures[i], 0 );
		}
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	}
	else
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
	
	//depth render buffer attachement
	if(this->depthType!=FBO_DepthBufferType_NONE)
	{
		switch(this->depthType)
		{
		case FBO_DepthBufferType_TEXTURE:
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, this->depthID, 0);
			break;

		case FBO_DepthBufferType_RENDERTARGET:
		default:
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, this->depthID);
			break;
		}
	}

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	CheckFramebufferStatus(this->fbo);
}

FrameBufferObjectMRT::~FrameBufferObjectMRT()
{
	if(this->nbColorAttachement>0)
	{
		delete [] this->colorTextures;
		delete [] this->colorMinificationFiltering;
	}
}



//////////////////////////////////////////////////////////////////////////////////



const GLint FrameBufferObjectMRT::getMaxColorAttachments()
{
  GLint maxAttach = 0;
  glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS_EXT, &maxAttach );
  return maxAttach;
}
const GLint FrameBufferObjectMRT::getMaxBufferSize()
{
  GLint maxSize = 0;
  glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE_EXT, &maxSize );
  return maxSize;
}



void FrameBufferObjectMRT::enableRenderToColorAndDepth(const unsigned int colorBufferNum) const
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, this->fbo);

	if(this->nbColorAttachement>0)
	{
		unsigned int colorBuffer = GL_COLOR_ATTACHMENT0_EXT + colorBufferNum;
		if((int)colorBufferNum>this->nbColorAttachement)
			colorBuffer = GL_COLOR_ATTACHMENT0_EXT;
		glDrawBuffer(colorBuffer);
	}
	else
	{
		glDrawBuffer(GL_NONE);	//for example, in the case of rendering in a depth buffer
	}
}

void FrameBufferObjectMRT::enableRenderToColorAndDepth_MRT(const GLuint numBuffers, const GLenum* drawbuffers) const
{ 
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, this->fbo);

	//HERE, THERE SHOULD HAVE A TEST OF THE DRAW BUFFERS ID AND NUMBER
	/*for(GLuint i=0; i<numBuffers; i++)
	{
	}*/ 
    glDrawBuffers(numBuffers, drawbuffers);

}

void FrameBufferObjectMRT::disableRenderToColorDepth()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}



void FrameBufferObjectMRT::saveAndSetViewPort() const
{
	glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, this->width, this->height);
}

void FrameBufferObjectMRT::restoreViewPort() const
{
	glPopAttrib();
}



void FrameBufferObjectMRT::bindColorTexture(const unsigned int colorBufferNum) const
{
	if(this->nbColorAttachement>0 && (int)colorBufferNum<this->nbColorAttachement)
	{
		glBindTexture(GL_TEXTURE_2D, this->colorTextures[colorBufferNum]);
	}
	else
		glBindTexture(GL_TEXTURE_2D, 0);
}

void FrameBufferObjectMRT::bindDepthTexture() const
{
	if(this->depthType==FBO_DepthBufferType_TEXTURE)
	{
		glBindTexture(GL_TEXTURE_2D, this->depthID);
	}
	else
		glBindTexture(GL_TEXTURE_2D, 0);
}

void FrameBufferObjectMRT::generateColorBufferMipMap(const unsigned int colorBufferNum) const
{
	if(this->nbColorAttachement>0 && (int)colorBufferNum<this->nbColorAttachement)
	{
		if(this->colorMinificationFiltering[colorBufferNum]==GL_NEAREST
		|| this->colorMinificationFiltering[colorBufferNum]==GL_LINEAR)
			return;	//don't allow to generate mipmap chain for texture that don't support it at the creation

		glBindTexture(GL_TEXTURE_2D, this->colorTextures[colorBufferNum]);
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}
}

void FrameBufferObjectMRT::generateDepthBufferMipMap() const
{
	if(this->depthType==FBO_DepthBufferType_TEXTURE)
	{
		if(this->depthMinificationFiltering==GL_NEAREST
		|| this->depthMinificationFiltering==GL_LINEAR)
			return;	//don't allow to generate mipmap chain for texture that don't support it at the creation

		glBindTexture(GL_TEXTURE_2D, this->depthID);
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}
}



unsigned int FrameBufferObjectMRT::getNumberOfColorAttachement() const
{
	return this->nbColorAttachement;
}

FBO_DepthBufferType FrameBufferObjectMRT::getDepthBufferType() const
{
	return this->depthType;
}

unsigned int FrameBufferObjectMRT::getWidth() const
{
	return this->width;
}

unsigned int FrameBufferObjectMRT::getHeight() const
{
	return this->height;
}

/*
void FrameBufferObjectMRT::saveToDisk(const unsigned int colorBufferNum, const char* filepath) const
{
	if(this->nbColorAttachement>0 && (int)colorBufferNum<this->nbColorAttachement)
	{
		ImageBMP::TextureToDisk(filepath,GL_TEXTURE_2D,this->colorTextures[colorBufferNum]);
	}
}*/