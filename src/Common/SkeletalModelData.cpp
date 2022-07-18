/***
*
*	License here.
*
*	@file
*
*	Skeletal Animation functionality.
*
***/
#include "Shared/Shared.h"
#include "Common/Files.h"


/**
*
*	Debug Configurations: 
*
*	In order to prevent an annoying flood of information to seek through, there is a unique 
*	define to enable debug output specifically for each section of the data generation process.
*
**/
#define DEBUG_MODEL_DATA 0
#define DEBUG_MODEL_BOUNDINGBOX_DATA 0


/**
*
*
*	Human Friendly Skeletal Model Data. Cached by server and client.
*
*
**/
/**
*	@return	Game compatible skeletal model data including: animation name, 
*			and frame data, bounding box data(per frame), and joints(name, index, parentindex)
**/
void SKM_GenerateModelData(model_t* model) {
	if (!model || !model->iqmData || !model->skeletalModelData) {
		// TODO: Warn.
		
		return ;
	}

	// Get SkeletalModelData ptr.
	SkeletalModelData *skm = model->skeletalModelData;

	/**
	*	Joints:
	**/
	// Get joint names sorted for indexing.
	char *jointNames = model->iqmData->jointNames;
	std::vector<std::string> parsedJointNames{};

	if (jointNames) {
			int32_t nameLength = strlen(jointNames);
			int32_t id = 0;
			while (nameLength) {
				// Push back next joint name.
				parsedJointNames.push_back(jointNames);
				// Increase index ofc.
				jointNames += nameLength + 1;
				// Fetch next name block length.
				nameLength = strlen(jointNames);
			}

		// First store our actual number of joints.
		skm->numberOfJoints = model->iqmData->num_joints;
	} else {
		skm->numberOfJoints = 0;
	}

	// Get our Joint data sorted out nicely.
	for (int32_t jointIndex = 0; jointIndex < model->iqmData->num_joints; jointIndex++) {
		// Get the parent joints.
		const int32_t jointParentIndex = (jointIndex == 0 ? -1 : model->iqmData->jointParents[jointIndex]);

		// Create our joint object.
		SkeletalModelData::Joint jointData = {
			.name = parsedJointNames[jointIndex],
			.index = jointIndex,
			.parentIndex = jointParentIndex,
		};
		// Store our joint in our list.
		skm->jointMap[jointData.name] = jointData;
		if (jointData.index < 256) {
			skm->jointArray[jointData.index] = jointData;
		} else {
			// TODO: Warn.
		}

		// If we are dealing with the "root" bone, store it.
		if (jointData.name == "mixamorig8:Hips") {
			skm->rootJointIndex = jointIndex;
		}
	}
// Debug Info:
#if DEBUG_MODEL_DATA == 1
	for (int32_t i = 0; i < skm->numberOfJoints; i++) {
		SkeletalModelData::Joint *joint = &skm->jointArray[i];
		
		if (joint->parentIndex >= 0 && joint->parentIndex < skm->numberOfJoints) {
			SkeletalModelData::Joint *parentJoint = &skm->jointArray[joint->parentIndex];

			Com_DPrintf("Joint(#%i, %s): parentJoint(%i, %s)\n",
				joint->index,
				joint->name.c_str(),
				joint->parentIndex,
				parentJoint->name.c_str()
			);
		} else {
			Com_DPrintf("Joint(#%i, %s): parentJoint(none)\n",
				joint->index,
				joint->name.c_str()
			);			
		}
	}
#endif

	/**
	*	Animations:
	**/
// Debug Info:
#if DEBUG_MODEL_DATA == 2
	Com_DPrintf("Animations:\n");
#endif
	// Get our animation data sorted out nicely.
	for (uint32_t animationIndex = 0; animationIndex < model->iqmData->num_animations; animationIndex++) {
		// Raw Animation Data.
		iqm_anim_t* animationData = &model->iqmData->animations[animationIndex];

		// Game Friendly Animation Data.
		// TODO: Do proper error checking for existing keys and warn.
		SkeletalAnimation *animation = &(skm->animationMap[animationData->name] = {
			.index = animationIndex,
			.name = animationData->name,
			.startFrame = animationData->first_frame,
			.endFrame = animationData->first_frame + animationData->num_frames,
			.numFrames = animationData->num_frames,
			.frametime = BASE_FRAMETIME,
			.loopingFrames = 0,
			.forceLoop = true, //(animationData->loop == 1 ? true : false)
		 });
		// Resize our vec if needed.
		if (skm->animations.size() <= animationIndex) {
			skm->animations.resize(animationIndex + 1);
		}
		skm->animations[animationIndex] = animation;

		// Calculate distances.
		if (skm->rootJointIndex != -1 && model->iqmData && model->iqmData->poses) {
			// Start and end pose pointers.
			const iqm_transform_t *startPose = &model->iqmData->poses[ skm->rootJointIndex + ( animation->startFrame * model->iqmData->num_poses ) ];
			const iqm_transform_t *endPose = (animation->startFrame == 0 ? startPose : &model->iqmData->poses[skm->rootJointIndex + ( (animation->endFrame - 1) * model->iqmData->num_poses)] );

			// Get the start and end pose translations.
			const vec3_t startFrameTranslate	= startPose->translate;
			const vec3_t endFrameTranslate		= endPose->translate;

			// Used to store the total translation distance from startFrame to end Frame,
			// We use this in order to calculate the appropriate distance between start and end frame.
			// (Ie, assuming an animation loops, we need that.)
			vec3_t totalTranslateDistance = vec3_zero();

			// The offset between the previous processed and the current processing frame.
			vec3_t offsetFrom = vec3_zero();

			for (int32_t i = animation->startFrame; i < animation->endFrame; i++) {
				// Get the Frame Pose.
				const iqm_transform_t *framePose = &model->iqmData->poses[skm->rootJointIndex + (i * model->iqmData->num_poses)];

				// Special Case: First frame has no offset really.
				if (i == animation->startFrame) {
					//const vec3_t totalStartTranslation = endFrameTranslate - startFrameTranslate;
					// Push the total traversed frame distance.
					const vec3_t frameTranslate = offsetFrom - framePose->translate;

					animation->frameDistances.push_back( vec3_length( frameTranslate ) );

					// Push the total translation between each frame.					
					animation->frameTranslates.push_back( frameTranslate );

					// Prepare offsetFrom with the current pose's translate for calculating next frame.
					offsetFrom = framePose->translate;

					//totalTranslationSum += totalStartTranslation;
				// Special Case: Take the total offset, subtract it from the end frame, and THEN
				
				//} else if (i == animation->endFrame) {
				//	// 
				//	const vec3_t frameTranslate = startFrameTranslate - endFrameTranslate; //*offsetFrom -*/ framePose->translate;
				//	const double frameDistance = vec3_distance_squared( startFrameTranslate, endFrameTranslate ); 

				//	const vec3_t totalBackTranslation = frameTranslate - offsetFrom; //startFrameTranslate - endFrameTranslate;
				//	//const vec3_t totalBackTranslation = startFrameTranslate - endFrameTranslate;

				//	// Push the total traversed frame distance.
				//	animation->frameDistances.push_back( frameDistance );//vec3_dlength( totalBackTranslation ) );

				//	// Push the total translation between each frame.					
				//	animation->frameTranslates.push_back( totalBackTranslation );

					// Calculate the full animation distance.
					//animation->animationDistance = vec3_distance_squared( endFrameTranslate, startFrameTranslate ); 

					//totalTranslationSum += totalBackTranslation;	
				// General Case: Set the offset we'll be coming from next frame.
				} else {
						
					// Calculate translation between the two frames.
					const vec3_t translate = offsetFrom - framePose->translate;
					//const vec3_t translate = offsetFrom - framePose->translate;

					// Calculate the distance between the two frame translations.
					const double frameDistance = vec3_distance_squared( offsetFrom, framePose->translate ); //vec3_dlength( translate );

					// Push the total traversed frame distance.
					animation->frameDistances.push_back( frameDistance );

					// Push the total translation between each frame.					
					animation->frameTranslates.push_back( translate );

					// Increment our total translation sum.
					//totalTranslationSum += translate; // or offsetfrom?

					// Prepare offsetFrom with the current pose's translate for calculating next frame.
					offsetFrom = framePose->translate;
				}
			}

			// Sum up all frame distances into one single value.
			//vec3_dlength(totalTranslationSum); //0.0;
			animation->animationDistance = 0.0;
			for (auto& distance : animation->frameDistances) {
				animation->animationDistance += distance;
			}
		}

#if DEBUG_MODEL_DATA == 1
		Com_DPrintf("Animation(#%i, %s): (startFrame=%i, endFrame=%i, numFrames=%i), (loop=%s, loopFrames=%i), (animationDistance=%f):\n",
			animationIndex,
			animationData->name, //animation.name, Since, temp var and .c_str()
			animation->startFrame,
			animation->endFrame,
			animation->numFrames,
			animation->forceLoop == true ? "true" : "false",
			animation->loopingFrames,
			animation->animationDistance);

		for (int i = 0; i < animation->frameDistances.size(); i++) {
			// Debug OutPut:
			int32_t frameIndex = i;
			Com_DPrintf("	Frame(#%i): Translate=(%f,%f,%f), Distance=%f\n", 
				frameIndex,
				animation->frameTranslates[frameIndex].x,
				animation->frameTranslates[frameIndex].y,
				animation->frameTranslates[frameIndex].z,
				animation->frameDistances[frameIndex]						
			);
		}
#endif
	}

	/**
	*	Model Bounds:
	**/
// Debug info:
#if DEBUG_MODEL_BOUNDINGBOX_DATA == 1
	Com_DPrintf("BoundingBoxes:\n");
#endif
	// Get the Model Bounds for each frame.
	float *bounds = model->iqmData->bounds;

	if (bounds) {
		for (int32_t frameIndex = 01; frameIndex < model->iqmData->num_frames; frameIndex++) {
			SkeletalModelData::BoundingBox box;
			box.mins = { bounds[0], bounds[1], bounds[2] };
			box.maxs = { bounds[3], bounds[4], bounds[5] };
			bounds+= 6;
		
	// Debug Info:
#if DEBUG_MODEL_BOUNDINGBOX_DATA == 1
	//Com_DPrintf("	Frame (#%i): (mins.x=%f, mins.y=%f, mins.z=%f), (maxs.x=%f, maxs.y=%f, maxs.z=%f)\n",
	//	frameIndex,
	//	box.mins.x, box.mins.y, box.mins.z,
	//	box.maxs.x, box.maxs.y, box.maxs.z
	//);
#endif
			skm->boundingBoxes.push_back(box);
		}
	}

	// Return our data.
	//return skm;
}

/**
*
*
*
*	Animation Configuration Parsing:
*	
*	Takes care of parsing animation configuration files such as this example:
*
*	----
*	rootbone "mixamo8:Spine" 
*
*	headbone "mixamo8:neckthing" 
*	torsobone "mixamo8:Spine1" 
*
*	action "idle" 0 1 -1 16.66 
*	action "run" 23 64 -1 16.66 
*	action "reload" 65 95 -1 16.66 
*
*	animation "RunAndReload" 
*		blendaction "run" 1.0 
*		blendaction "reload" 1.0 torsobone 
*	----
*
*	The following 'commands' are available:
*	'rootbone': Specifies which bone acts as a rootmotion movement origin point.
*				Can be identified by a string name, or integral number index.
*
*	'headbone':	Specifies where the head starts in case of a humanoid model.
*				Can be identified by a string name, or integral number index.
*	'torsobone':Specifies where the torso starts in case of a humanoid model.
*				Can be identified by a string name, or integral number index.
*
*	'action':	Actions define specific animation frame sets by assigning them
*				a unique distinct name, startframe, endframe, loopCount, and frameTime.
*
*	'animation':Animations are composed by packing 'blendaction' commands together. 
*				The first blend 'takes the lead', and is considered the dominating
*				animation on which we blend other animations into.
*				
*	'blendaction':	Assigns an 'action' to a bone node with the given blend fraction.
*					If no bonenode is present, it is regarded as the leading dominator
*					animation instead.
*				
*
*
**/
//! Contains a parsed token.
struct SKMParsedToken {
	//! Token Flags.
	struct Flags {
		//! This flag is set whenever a an opening double quote has matched an ending double quote.
		static constexpr int32_t QuotedString = 1 << 1;
		//! This flag is set when the token passed an integral number test.
		static constexpr int32_t IntegralNumber = 1 << 2;
		//! This flag is set when the token passed a floating point number test.
		static constexpr int32_t FloatNumber = 1 << 3;
		//! This flag is set if the token is contained within our commands list.
		static constexpr int32_t CommandIdentifier = 1 << 4;
		//! This flag is set when all of the above have failed, we'll assume that it is an identifier of sorts.
		static constexpr int32_t Identifier = 1 << 5;
	};
	//! Line number we found this token at.
	int32_t line = 0;
	//! Character offset number into our line at which we found this token at.
	int32_t offset = 0;
	//! Token character width. (To avoid having to request string size all the time.)
	int32_t width = 0;

	//! Actual token string.
	std::string value = "";
	//! Keeps track of specific token flags.
	int32_t flags = 0;	// These are set to either cmd, string(if it was encased by quotes), int, float.
						// In case of cmd, also a cmd index is set.
};

//! Stores the final parsed results of each line.
struct SKMParsedLine {
	//! Actual line number.
	int32_t number = 0;
	
	//! Total character count (width) of the line.
	int32_t width = 0;

	//! String value containing line data.
	std::string value = "";

	//! Filled with 'categorized' type tokens found in the line's string value.
	std::vector<SKMParsedToken> tokens;
};

//! Contains our parsed tokens.
struct SKMParsedResults {
	std::vector<SKMParsedToken> tokens;
};

//! String list of all our existing commands.
static std::vector< std::string > skmCommandList = {
	"rootbone",
	"headbone",
	"torsobone",
	"action",
	"animation",
	"blendaction",
};


///*********
//*	Temporary: TODO: Move to string header? Maybe make more utilities, look into the tokenizing process and C++-ify that.
//**/
#include <regex>
// This regex splitter does not seem to work, unsure if it is our fault or MS.
static inline std::vector<std::string> RegExSplitter(std::string input) {
    std::regex output("(? : ^|s *)(S + )");
	//"^[ |\t|\n|\r|\v|\f]*|[ |\t|\n|\r|\v|\f]*$" 
    return { std::sregex_token_iterator(std::cbegin(input), std::cend(input), output, 1), std::sregex_token_iterator() };
}
static inline const std::string StringRegex( const std::string &str, const std::string &regularExpression = "" ) {
	return std::regex_replace(str, std::regex(regularExpression), "");
}
static inline const std::string StringRegexTrim( const std::string &str, const std::string &regularExpression = "^[ |\t|\n|\r|\v|\f]*|[ |\t|\n|\r|\v|\f]*$" ) {
	return StringRegex( str, regularExpression );
}
//
///**
//*	End of Temporary.
//*********/




/**
*	@return	The flag(s) categorizing(describing the intent) of this token's string content.
**/
static int32_t SKM_CategorizeTokenValue( const std::string &tokenValue ) {
	// Flags that tell us what the actual content of this token is.
	int32_t flagsMask = 0;

	/**
	*	#0: Test whether this token is any of our accepted commands, if it is, skip any other tests.
	**/
	// Test whether it is in our commands list.
	const bool isCommand = std::any_of(skmCommandList.begin(), skmCommandList.begin() + skmCommandList.size(), [&tokenValue](const std::string &cmd) {
			return cmd == tokenValue; 
		} 
	);

	/**
	*	#1: Proceed with other tests if not a command, otherwise apply CommandIdentifier flag.
	**/
	// If it is a command, set command identifier flag and skip other tests.
	if ( isCommand ) {
		flagsMask |= SKMParsedToken::Flags::CommandIdentifier;
	// It was no command, proceed for further testing.
	} else {
		// Test for strictly integral number.
		if ( tokenValue.find_first_not_of("-0123456789") == std::string::npos ) {		
			flagsMask |= SKMParsedToken::Flags::IntegralNumber;
		// Test for the '.', if it is there, we expect a floating point number.
		} else if ( tokenValue.find_first_not_of(".-0123456789") == std::string::npos ) {
			flagsMask |= SKMParsedToken::Flags::FloatNumber;
		// It's none of the above, so we assume it is an unnamed identifier.
		} else {
			flagsMask |= SKMParsedToken::Flags::Identifier;
		}
	}

	// Return flagsMask.
	return flagsMask;
}

/**
*	@brief	Tokenizes the SKMParsedLine its value and 
**/
static bool SKM_TokenizeParsedLine( SKMParsedLine &parsedLine, std::string &errorString ) {
	/**
	*	
	**/
    std::size_t previousPosition = 0;
    std::size_t currentPosition = 0;
 
	while ((currentPosition = parsedLine.value.find_first_of(" \t", previousPosition)) != std::string::npos) {
        if ( currentPosition > previousPosition ) {
			// Calculate offset.
			const int32_t tokenOffset = previousPosition;
			// Calculate width.
			const int32_t tokenWidth = currentPosition - previousPosition;
			// Take token value.
			const std::string tokenValue = parsedLine.value.substr( tokenOffset, tokenWidth );
			// Determine special token flags for this token.
			const int32_t tokenFlags = SKM_CategorizeTokenValue( tokenValue );

			parsedLine.tokens.emplace_back( SKMParsedToken{
				.line = parsedLine.number,
				.offset = tokenOffset,
				.width = tokenWidth,
				.value = tokenValue,
				.flags = tokenFlags
			});
        }
        previousPosition = currentPosition + 1;
    }

	// Take care of the last but of string value.
	if ( previousPosition < parsedLine.value.length() ) {
		// Calculate offset.
		const int32_t tokenOffset = previousPosition;
		// Calculate width.
		const int32_t tokenWidth = parsedLine.value.length() - tokenOffset;
		// Take token value.
		const std::string tokenValue = parsedLine.value.substr( tokenOffset );
		// Determine special token flags for this token.
		const int32_t tokenFlags = SKM_CategorizeTokenValue( tokenValue );

        parsedLine.tokens.emplace_back( SKMParsedToken{
			.line = parsedLine.number,
			.offset = tokenOffset,
			.width = tokenWidth,
			.value = tokenValue,
			.flags = tokenFlags
		});
    }

	return true;
}

/**
*	@brief	Splits the buffer into separate distinct lines. Feeding them to 'parsedLines'.
*	@param	buffer The buffer to scan and split up into separate distinct lines.
*	@param	&lineTokens	A vector reference to push back all parsed lines on to.
*	@return	The total amount of lines found in 'buffer'.
**/
static int32_t SKM_SplitLines( const std::string &buffer, std::vector<SKMParsedLine> &parsedLines) {
	//! Keeps track of total lines found.
	int32_t totalLineCount = 0;

    std::size_t previousPosition = 0;
    std::size_t currentPosition = 0;
 
	while ((currentPosition = buffer.find_first_of( "\r\n", previousPosition)) != std::string::npos) {
        if ( currentPosition > previousPosition ) {
			// Actual size of line to be pushed.
			const int32_t lineSize = currentPosition - previousPosition;

			// Emplace our parsed line data.
			parsedLines.emplace_back(SKMParsedLine{
				.number = totalLineCount,
				.width = lineSize,
				// Trim the substring from all things that cause white spaces, on both left and right sides.
				.value = StringRegexTrim(
					buffer.substr( previousPosition, currentPosition - previousPosition )
				)
			});

			// Increment line count.
			totalLineCount++;
        }
        previousPosition = currentPosition + 1;
    }

    if ( previousPosition < buffer.length() ) {
		// Actual size of line to be pushed.
		const int32_t lineSize = previousPosition;

		// Emplace our parsed line data.
		parsedLines.emplace_back(SKMParsedLine{
			.number = totalLineCount,
			.width = lineSize,
			// Trim the substring from all things that cause white spaces, on both left and right sides.
			.value = StringRegexTrim(
				buffer.substr( previousPosition )
			)
		});


		// Increment line count.
		totalLineCount++;
    }

	// Return total lines found.
	return totalLineCount;
}

/**
*	@brief	Parsed a Skeletal Model Configuration file for animation actions, tag and blend data.
**/
static bool SKM_ParseConfiguration( const std::string &cfgBuffer ) {
	// Our parsed line container which we first parse all lines, then
	// tokenize each line and store its tokens.
	std::vector<SKMParsedLine> parsedLines;

	/**
	*	#0:	First split by '\r\n' feeds.
	**/
	int32_t totalLinesParsed = SKM_SplitLines( cfgBuffer, parsedLines );

	/**
	*	#1: Now actually tokenize each line, storing the tokens in the parsed line.
	**/
	for ( auto &parsedLine : parsedLines ) {
		// This string is set in case of any errors.
		std::string errorString = "";

		// Tokenize the current line.
		if ( !SKM_TokenizeParsedLine( parsedLine, errorString ) ) {
			// Something went wrong, output error.
		}
	}
	// Debug output for testing tokenizing.
	Com_DPrintf("===================================================\n");
	Com_DPrintf("Tokenizing into lines, and each line into tokens based on spaces and tabs:\n");
	// Iterate our tokenized line buffers.
	for (auto &parsedLine : parsedLines ) {
	//	// Tokenize this line for spaces and tabs.
	//	auto lineTokens = SKM_StringTokenize( bufferLine, " \t" );

	//	// Print line:
		Com_DPrintf( "#%i:%s: {\n", parsedLine.number, parsedLine.value.c_str() );
		for ( auto &token : parsedLine.tokens ) {

			std::string flagStr = "";

			// Finish generating our debug flag mask by testing and adding string counterparts for all remaining 
			// flags other than the QuoteString flag.
			if ( token.flags & SKMParsedToken::Flags::CommandIdentifier )	{ flagStr += "CommandIdentifier "; }
			if ( token.flags & SKMParsedToken::Flags::FloatNumber )			{ flagStr += "FloatNumber "; }
			if ( token.flags & SKMParsedToken::Flags::IntegralNumber )		{ flagStr += "IntegralNumber "; }
			if ( token.flags & SKMParsedToken::Flags::QuotedString )		{ flagStr += "QuotedString "; }
			if ( token.flags & SKMParsedToken::Flags::Identifier )			{ flagStr += "Identifier "; }

			// Do some debug output.
			Com_DPrintf("   token: {%s}, flags: {%s} \n", token.value.c_str(), flagStr.c_str() );
		}
		// Debug output.
		Com_DPrintf("}\n");
	}
	
	// Debug output for testing tokenizing.
	Com_DPrintf("===================================================\n");

	return true;
}

/**
*	@brief	Loads up a skeletal model configuration file and passes its buffer over
*			to the parsing process. The process tokenizes the data and generates game
*			code friendly POD to work with.
**/
bool SKM_LoadAndParseConfiguration( const std::string &filePath ) {
    // Stores the actual buffer content.
	char *fileBuffer; //, * data, * p;// , * cmd;
    qerror_t ret = 0;

	// Load file into buffer.
	ret = FS_LoadFile( filePath.c_str(), (void **)&fileBuffer );

    if ( !fileBuffer ) {
        if ( ret != Q_ERR_NOENT ) {
			// Generate our development output string using fmt.
			const std::string devPrintStr = fmt::format(
				"Couldn't load '{}': {}",
				filePath,
				Q_ErrorString( ret )
			);
			// Output.
			Com_DPrintf(devPrintStr.c_str());
        }
		// Return failure.
        return false;
    }

	// Start parsing our buffer.
	SKM_ParseConfiguration( fileBuffer );

	// We're done working with this file, free it from memory.
	FS_FreeFile( fileBuffer );

	// Return success.
	return true;
}