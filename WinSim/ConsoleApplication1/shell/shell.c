/**
 * @file shell.c
 * @author Letter (NevermindZZT@gmail.com)
 * @version 3.0.0
 * @date 2019-12-30
 * 
 * @Copyright (c) 2020 Letter
 * 
 */

#include "shell.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "shell_ext.h"


#if SHELL_USING_CMD_EXPORT == 1
/**
 * @brief 濠殿喗甯楃粙鎺椻�﹂崼銉晣濠电姵纰嶉崑銊╂煏婵犲繒鐣遍柣鎿勬嫹
 */
const char shellCmdDefaultUser[] = SHELL_DEFAULT_USER;
const char shellPasswordDefaultUser[] = SHELL_DEFAULT_USER_PASSWORD;
const char shellDesDefaultUser[] = "defalut user";
const ShellCommand shellUserDefault SECTION("shellCommand") =
{
    .attr.value = SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_USER),
    .data.user.name = shellCmdDefaultUser,
    .data.user.password = shellPasswordDefaultUser,
    .data.user.desc = shellDesDefaultUser
};
#endif

#if SHELL_USING_CMD_EXPORT != 1
    extern const ShellCommand shellCommandList[];
    extern const unsigned short shellCommandCount;
#endif

    extern int loc_fileSize;
    extern unsigned char * pCacheBuffer;
/**
 * @brief shell 闂佹眹鍩勯崹閬嶅箖閸岀偛闂繛宸簻濡﹢鏌涢妷锝呭闁圭柉浜槐鎺撶瑹閸喚浠肩紓浣瑰敾閹凤拷
 */
enum
{
    SHELL_TEXT_INFO,                                    /**< shell濠电儑绲藉ú鐘诲礈濠靛洤顕遍柨鐕傛嫹 */
    SHELL_TEXT_CMD_TOO_LONG,                            /**< 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濇娴溿倝鏌涢妷顔煎婵綇鎷� */
    SHELL_TEXT_CMD_LIST,                                /**< 闂備礁鎲￠悷顖炲垂鏉堛劍娅犻柣锝呮湰閸嬫﹢鎮橀悙璺盒撻柛銈嗗笒闇夐柣姗嗗枛閿熻姤娲熷畷褰掝敂閸℃ê浠掗梺闈涱煭婵″洭宕归悡骞稓鏁钘夘伓 */
    SHELL_TEXT_VAR_LIST,                                /**< 闂備礁鎲￠悷锕傛晝閵忋倕闂繛宸簻缁�鍡樹繆閵堝懎顏ラ柨鐔绘閸熸潙顕ｆ禒瀣倞妞ゆ帒顦板В锟� */
    SHELL_TEXT_USER_LIST,                               /**< 闂備胶鍎甸崑鎾诲礉瀹ュ鏄ユ繛鎴欏灩缁�鍡樹繆閵堝懎顏ラ柨鐔绘閸熸潙顕ｆ禒瀣倞妞ゆ帒顦板В锟� */
    SHELL_TEXT_KEY_LIST,                                /**< 闂備礁婀遍…鍫ニ囬悽绋挎瀬妞ゆ挶鍨圭粈鍡樹繆閵堝懎顏ラ柨鐔绘閸熸潙顕ｆ禒瀣倞妞ゆ帒顦板В锟� */
    SHELL_TEXT_CMD_NOT_FOUND,                           /**< 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濐槸鐎氬銇勮箛鎾村櫣妞ゃ儲绻堥弻娑㈠箳閻愭潙顏� */
    SHELL_TEXT_POINT_CANNOT_MODIFY,                     /**< 闂備礁婀遍…鍫澝洪銏犵闁跨喓濮撮惌妤佹叏濡ゅ瀚归梺闈╃悼閸庛倗绮欐径濠庡悑闁告侗鍘肩粭鎺楁煟閻樿京绐旈柡鍛叀楠炲牓濡搁埡浣哄摋闂佽法鍣﹂幏锟� */
    SHELL_TEXT_VAL_CANNOT_MODIFY,                       /**< 闂佹眹鍩勯崹閬嶅箖閸岀偛闂柣鎴烆焽閳绘梻锟藉箍鍎遍幊搴ｆ暜閹烘鍋ｅ〒姘炬嫹闁哄懏鐓￠獮鍫ュΩ閳轰胶鍝楅梺璺ㄥ櫐閹凤拷 */
    SHELL_TEXT_NOT_VAR,                                 /**< 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濇閳绘梻锟藉箍鍎卞Λ娆戠懅闂備礁鎲￠悷锕傛晝閵忋倕闂柨鐕傛嫹 */
    SHELL_TEXT_VAR_NOT_FOUND,                           /**< 闂備礁鎲￠悷锕傛晝閵忋倕闂繛宸簻鐎氬銇勮箛鎾村櫣妞ゃ儲绻堥弻娑㈠箳閻愭潙顏� */
    SHELL_TEXT_HELP_HEADER,                             /**< help濠电姰鍨奸鎰板箯閿燂拷 */
    SHELL_TEXT_PASSWORD_HINT,                           /**< 闂佽閰ｅ褍螞濞戙垺鍋夋繝濠傚缂嶅洭鏌熼幆褍鏆辩�殿喗濞婇弻鐔虹磼濡搫顫庨梺闈涙祫閹凤拷 */
    SHELL_TEXT_PASSWORD_ERROR,                          /**< 闂佽閰ｅ褍螞濞戙垺鍋夋繝濠傜墛閻撱儲绻涢崱妯轰刊闁搞倧鎷� */
    SHELL_TEXT_CLEAR_CONSOLE,                           /**< 婵犵數鍋為幐鎼佸箠閹版澘鐓橀柡宥庡幖缁犲磭鎲稿澶婃槬婵°倕鎳庨惌妤呮煥閻曞倹瀚� */
    SHELL_TEXT_TYPE_CMD,                                /**< 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濇川鐏忕敻鎮归崶顏勭毢闁跨喐鏋婚幏锟� */
    SHELL_TEXT_TYPE_VAR,                                /**< 闂備礁鎲￠悷锕傛晝閵忋倕闂柧蹇撴贡鐏忕敻鎮归崶顏勭毢闁跨喐鏋婚幏锟� */
    SHELL_TEXT_TYPE_USER,                               /**< 闂備胶鍎甸崑鎾诲礉瀹ュ鏄ユ繛鎴炴皑鐏忕敻鎮归崶顏勭毢闁跨喐鏋婚幏锟� */
    SHELL_TEXT_TYPE_KEY,                                /**< 闂備礁婀遍…鍫ニ囬悽绋挎瀬妞ゆ挾濮风亸鐢告偣閸ヮ亜鐨洪柨鐔告灮閹凤拷 */
    SHELL_TEXT_TYPE_NONE,                               /**< 闂傚倸鍊搁悧蹇涘磻閻愬灚鍏滈柛鎾茶兌鐏忕敻鎮归崶顏勭毢闁跨喐鏋婚幏锟� */
};



static const char *shellText[] =
{
#if 0
    [SHELL_TEXT_INFO] =
        "\r\n"
        " _         _   _                  _          _ _ \r\n"
        "| |    ___| |_| |_ ___ _ __   ___| |__   ___| | |\r\n"
        "| |   / _ \\ __| __/ _ \\ '__| / __| '_ \\ / _ \\ | |\r\n"
        "| |__|  __/ |_| ||  __/ |    \\__ \\ | | |  __/ | |\r\n"
        "|_____\\___|\\__|\\__\\___|_|    |___/_| |_|\\___|_|_|\r\n"
        "\r\n"
        "Build:       "__DATE__" "__TIME__"\r\n"
        "Version:     "SHELL_VERSION"\r\n"
        "Copyright:   (c) 2020 Letter\r\n",
#else
	[SHELL_TEXT_INFO] =
		"\r\n"
		"  #####     #    #     # ###  #####  ####### \r\n"
		" #     #   # #   #     #  #  #     # #       \r\n"
		" #        #   #  #     #  #  #       #       \r\n"
		" #       #     # #     #  #  #  #### #####   \r\n"
		" #       #######  #   #   #  #     # #       \r\n"
		" #     # #     #   # #    #  #     # #       \r\n"
		"  #####  #     #    #    ###  #####  ####### \r\n"
		"\r\n"
		"Build:       "__DATE__" "__TIME__"\r\n",
#endif
    [SHELL_TEXT_CMD_TOO_LONG] = 
        "\r\nWarning: Command is too long\r\n",
    [SHELL_TEXT_CMD_LIST] = 
        "\r\nCommand List:\r\n",
    [SHELL_TEXT_VAR_LIST] = 
        "\r\nVar List:\r\n",
    [SHELL_TEXT_USER_LIST] = 
        "\r\nUser List:\r\n",
    [SHELL_TEXT_KEY_LIST] =
        "\r\nKey List:\r\n",
    [SHELL_TEXT_CMD_NOT_FOUND] = 
        "Command not Found\r\n",
    [SHELL_TEXT_POINT_CANNOT_MODIFY] = 
        "can't set pointer\r\n",
    [SHELL_TEXT_VAL_CANNOT_MODIFY] = 
        "can't set val\r\n",
    [SHELL_TEXT_NOT_VAR] =
        " is not a var\r\n",
    [SHELL_TEXT_VAR_NOT_FOUND] = 
        "Var not Fount\r\n",
    [SHELL_TEXT_HELP_HEADER] =
        "command help of ",
    [SHELL_TEXT_PASSWORD_HINT] = 
        "\r\nPlease input password:",
    [SHELL_TEXT_PASSWORD_ERROR] = 
        "\r\npasswrod error\r\n",
    [SHELL_TEXT_CLEAR_CONSOLE] = 
        "\033[2J\033[1H",
    [SHELL_TEXT_TYPE_CMD] = 
        "CMD ",
    [SHELL_TEXT_TYPE_VAR] = 
        "VAR ",
    [SHELL_TEXT_TYPE_USER] = 
        "USER",
    [SHELL_TEXT_TYPE_KEY] = 
        "KEY ",
    [SHELL_TEXT_TYPE_NONE] = 
        "NONE",
};


/**
 * @brief shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁告洦鍘介崑姗�鏌ㄩ悤鍌涘
 */
static Shell *shellList[SHELL_MAX_NUMBER] = {NULL};


static void shellAdd(Shell *shell);
static void shellWriteCommandLine(Shell *shell);
static void shellWirteReturnValue(Shell *shell, int value);
static void shellShowVar(Shell *shell, ShellCommand *command);
static void shellSetUser(Shell *shell, const ShellCommand *user);


/**
 * @brief shell 闂備礁鎲＄敮妤冩崲閸岀儑缍栭柟鐗堟緲缁�宀勬煥閻曞倹瀚�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellInit(Shell *shell, char *buffer, unsigned short size)
{
    shell->parser.length = 0;
    shell->parser.cursor = 0;
    shell->history.offset = 0;
    shell->history.number = 0;
    shell->history.record = 0;
    shell->info.user = NULL;
    shell->status.isChecked = 1;

    shell->parser.buffer = buffer;
    shell->parser.bufferSize = size / (SHELL_HISTORY_MAX_NUMBER + 1);
    for (short i = 0; i < SHELL_HISTORY_MAX_NUMBER; i++)
    {
        shell->history.item[i] = buffer + shell->parser.bufferSize * (i + 1);
    }

#if SHELL_USING_CMD_EXPORT == 1
    #if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
        extern const unsigned int shellCommand$$Base;
        extern const unsigned int shellCommand$$Limit;

        shell->commandList.base = (ShellCommand *)(&shellCommand$$Base);
        shell->commandList.count = ((unsigned int)(&shellCommand$$Limit)
                                - (unsigned int)(&shellCommand$$Base))
                                / sizeof(ShellCommand);

    #elif defined(__ICCARM__)
        shell->commandList.base = (ShellCommand *)(__section_begin("shellCommand"));
        shell->commandList.count = ((unsigned int)(__section_end("shellCommand"))
                                - (unsigned int)(__section_begin("shellCommand")))
                                / sizeof(ShellCommand);
    #elif defined(__GNUC__)
        extern const unsigned int _shell_command_start;
        extern const unsigned int _shell_command_end;
        
        shell->commandList.base = (ShellCommand *)(&_shell_command_start);
        shell->commandList.count = ((unsigned int)(&_shell_command_end)
                                - (unsigned int)(&_shell_command_start))
                                / sizeof(ShellCommand);
    #else
        #error not supported compiler, please use command table mode
    #endif
#else
    shell->commandList.base = (ShellCommand *)shellCommandList;
    shell->commandList.count = shellCommandCount;
#endif

    shellAdd(shell);

    shellSetUser(shell, shellSeekCommand(shell,
                                         SHELL_DEFAULT_USER,
                                         shell->commandList.base,
                                         0));
    shellWriteCommandLine(shell);
}




/**
 * @brief 婵犵數鍎戠紞锟芥い鏇嗗嫭鍙忛柣搴ｏ拷鎭妉l
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
static void shellAdd(Shell *shell)
{
    for (short i = 0; i < SHELL_MAX_NUMBER; i++)
    {
        if (shellList[i] == NULL)
        {
            shellList[i] = shell;
            return;
        }
    }
}


/**
 * @brief 闂備礁鍚嬮崕鎶藉床閼艰翰浜归柛銉簵娴滃綊鏌熼幆褍鏆辨い銈呮噺缁绘盯鎳犳０婵嗘婵犳鍣粻绌恊ll
 * 
 * @return Shell* 闁荤喐绮庢晶妤呭箰閸涘﹥娅犻柣妯款梿閾忚瀚氶柟缁樺醇濮婎櫘ell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
Shell* shellGetCurrent(void)
{
    for (short i = 0; i < SHELL_MAX_NUMBER; i++)
    {
        if (shellList[i] && shellList[i]->status.isActive)
        {
            return shellList[i];
        }
    }
    return NULL;
}


/**
 * @brief shell闂備礁鎲￠崝鏍偡閵夆晜鍋ょ憸蹇曞垝椤撱垺鏅搁柨鐕傛嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param data 闂佽瀛╃粙鎺椼�冮崼銉晞濞撴熬鎷风�殿噮鍋婇幃褔宕煎┑鍫涘亰
 */
static void shellWriteByte(Shell *shell, const char data)
{
    shell->write(data);
}


/**
 * @brief shell 闂備礁鎲￠崝鏍偡閵夆晜鍋ょ憸蹇曞垝椤撱垺鏅滈柤鎰佸灱濞诧拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param string 闂佽瀛╃粙鎺椼�冮崼銉晞濞达絽婀遍埢鏃堟煕濞嗗浚妲规繛鍫濈埣閺岀喖鎯傞崨濠傤伓
 * 
 * @return unsigned short 闂備礁鎲￠崝鏍偡閵夆晛鐭楅柛鈩冪懕閹峰嘲鈽夊▎妯荤暭濡炪倖鍨崇欢姘跺箚閸愵喖绀嬫い鎺嶇劍椤斿秹姊绘担绛嬪殝闁瑰嚖鎷�
 */
unsigned short shellWriteString(Shell *shell, const char *string)
{
    unsigned short count = 0;
    SHELL_ASSERT(shell->write, return 0);
    while(*string)
    {
        shell->write(*string ++);
        count ++;
    }
    return count;
}


/**
 * @brief shell 闂備礁鎲￠崝鏍偡閵夆晛绠犻柍鈺佸暟椤╃兘鏌曢崼婵囶棞濞寸厧閰ｅ鍫曞煛閸屾稓鍑￠梺鐑╁閸涱垳鐣堕梺鎸庢閸庡銆呴敓锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param string 闂佽瀛╃粙鎺椼�冮崼銉晞濞达絽婀遍埢鏃堟煕濞嗗浚妲规繛鍫濈埣閺岀喖鎯傞崨濠傤伓
 * 
 * @return unsigned short 闂備礁鎲￠崝鏍偡閵夆晛鐭楅柛鈩冪懕閹峰嘲鈽夊▎妯荤暭濡炪倖鍨崇欢姘跺箚閸愵喖绀嬫い鎺嶇劍椤斿秹姊绘担绛嬪殝闁瑰嚖鎷�
 */
static unsigned short shellWriteCommandDesc(Shell *shell, const char *string)
{
    unsigned short count = 0;
    SHELL_ASSERT(shell->write, return 0);
    while(*string
        && *string != '\r'
        && *string != '\n'
        && count < 36)
    {
        shell->write(*string ++);
        count ++;
        if (count >= 36 && *(string + 1))
        {
            shell->write('.');
            shell->write('.');
            shell->write('.');
        }
    }
    return count;
}


/**
 * @brief shell闂備礁鎲￠崝鏍偡閵夆晛绠犻柍鈺佸暟椤╃兘鏌曢崼婵囶棞缂佹彃婀辩槐鎺楁晸娴犲鎯為悹鍥ｏ拷鍐插⒕
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * 
 */
static void shellWriteCommandLine(Shell *shell)
{
    if (shell->status.isChecked)
    {
        shellWriteString(shell, "\r\n");
        shellWriteString(shell, shell->info.user->data.user.name);
        shellWriteString(shell, ":/$ ");
    }
    else
    {
        shellWriteString(shell, shellText[SHELL_TEXT_PASSWORD_HINT]);
    }
}


#if SHELL_PRINT_BUFFER > 0
/**
 * @brief shell闂備礁鎼粔鍫曞储瑜忓Σ鎰版晸閻樿尙顦遍梺鍝勭▉閸撴稓绱欐繝姘厱闁规惌鍨辩�氾拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param fmt 闂備礁鎼粔鍫曞储瑜忓Σ鎰版晸閻樿尙顦遍梺鍝勮癁閸涱喗鍠曠紓鍌欑劍椤ㄦ劗绱撳杈ㄥ暫闁跨噦鎷�
 * @param ... 闂備礁鎲￠悷銉╁磹瑜版帒姹查柨鐕傛嫹
 */
void shellPrint(Shell *shell, char *fmt, ...)
{
    char buffer[SHELL_PRINT_BUFFER];
    va_list vargs;

    SHELL_ASSERT(shell, return);

    va_start(vargs, fmt);
    vsnprintf(buffer, SHELL_PRINT_BUFFER - 1, fmt, vargs);
    va_end(vargs);
    
    shellWriteString(shell, buffer);
}
#endif


/**
 * @brief shell 婵犵妲呴崑鎾诲箯閻戣姤鐓涢悘鐐额嚙閸旀岸鏌熼弬銉ュ⒉缂佸倹甯℃俊鍫曞幢濡偐袣闂傚倸鍊哥�氭悂骞忛敓锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param command ShellCommand
 * 
 * @return signed char 0 闁荤喐绮庢晶妤呭箰閸涘﹥娅犻柣妯肩帛閸嬨劑鏌曟繝蹇曠暠闁绘挻娲熼弻娑滅疀濞戞瑦鏆犲┑鐙呮嫹濞达絿鍎ょ�氭岸鏌曟径鍫濆姕闁搞倖甯掗湁闁绘﹩鍠栭敓鑺ョ墪椤洭宕奸妷锔规寖闂佽法鍣﹂幏锟�
 * @return signec char -1 闁荤喐绮庢晶妤呭箰閸涘﹥娅犻柣妯肩帛閸嬨劑鏌曟繝蹇曠暠闁绘挻娲栭埥澶愬箻瀹曞泦銏ゆ煕濞嗗繑鍋ョ�殿喖鐏氬鍕償椤斿吋鐣介梻浣告啞缁嬫帡宕瑰┑瀣闁靛牆顦痪褔鏌涢幇闈涙灍妞ゅ函鎷�
 */
signed char shellCheckPermission(Shell *shell, ShellCommand *command)
{
    return ((!command->attr.attrs.permission
                || command->attr.attrs.type == SHELL_TYPE_USER
                || (command->attr.attrs.permission 
                    & shell->info.user->attr.attrs.permission))
            && (shell->status.isChecked
                || command->attr.attrs.enableUnchecked))
            ? 0 : -1;
}


/**
 * @brief int闂佸搫顦遍崕鎴﹀箯閿燂拷16闂佸搫顦弲婊呯矙閹达箑鏄ユ俊銈呭枦閹峰嘲鈽夊▎妯荤暭濡炪倖鍨抽悞锔剧矙婢舵劖鏅搁柨鐕傛嫹
 * 
 * @param value 闂備浇妗ㄩ悞锕�顭垮锟介弫鎾诲棘閵堝棗顏�
 * @param buffer 缂傚倸鍊搁崐褰掑箰閹间礁鐤柨鐕傛嫹
 * 
 * @return signed char 闂佸搫顦遍崕鎰板礈濮橆剛鏆﹂柡灞诲劚鐟欙箓骞栧ǎ顒�鐏紒鐘冲浮閺屸剝鎷呴崫銉ヮ杸闂佸搫妫涢崰鏍嵁閹达富鏁嬮柨鐔剁矙瀵櫕娼忛妸锕�鍔呴梺璺ㄥ櫐閹凤拷
 */
signed char shellToHex(unsigned int value, char *buffer)
{
    char byte;
    unsigned char i = 8;
    buffer[8] = 0;
    while (value)
    {
        byte = value & 0x0000000F;
        buffer[--i] = (byte > 9) ? (byte + 87) : (byte + 48);
        value >>= 4;
    }
    return 8 - i;
}


/**
* @brief int闂佸搫顦遍崕鎴﹀箯閿燂拷10闂佸搫顦弲婊呯矙閹达箑鏄ユ俊銈呭枦閹峰嘲鈽夊▎妯荤暭濡炪倖鍨抽悞锔剧矙婢舵劖鏅搁柨鐕傛嫹
 * 
 * @param value 闂備浇妗ㄩ悞锕�顭垮锟介弫鎾诲棘閵堝棗顏�
 * @param buffer 缂傚倸鍊搁崐褰掑箰閹间礁鐤柨鐕傛嫹
 * 
 * @return signed char 闂佸搫顦遍崕鎰板礈濮橆剛鏆﹂柡灞诲劚鐟欙箓骞栧ǎ顒�鐏紒鐘冲浮閺屸剝鎷呴崫銉ヮ杸闂佸搫妫涢崰鏍嵁閹达富鏁嬮柨鐔剁矙瀵櫕娼忛妸锕�鍔呴梺璺ㄥ櫐閹凤拷
 */
signed char shellToDec(int value, char *buffer)
{
    unsigned char i = 11;
    int v = value;
    if (value < 0)
    {
        v = -value;
    }
    buffer[11] = 0;
    while (v)
    {
        buffer[--i] = v % 10 + 48;
        v /= 10;
    }
    if (value < 0)
    {
        buffer[--i] = '-';
    }
    if (value == 0) {
        buffer[--i] = '0';
    }
    return 11 - i;
}


/**
 * @brief shell闂佽瀛╃粙鎺椼�冮崼銉晞濞达絽婀遍埢鏃堟煙缂佹ê淇俊鎻掝煼閺屾盯骞掗悙鏉戭伓
 * 
 * @param dest 闂備胶鍎甸弲鈺呭窗濡ゅ懏鍋夐柨婵嗘祫閹峰嘲鈽夊▎妯荤暭濡炪倖鍨抽悞锔剧矙婢舵劖鏅搁柨鐕傛嫹
 * @param src 婵犵數濮嶉崟顐㈩潓闂佺儵濮嶉崨顖滅暥闂佹寧妫侀崕濠氥�呴敓锟�
 * @return unsigned short 闂佽瀛╃粙鎺椼�冮崼銉晞濞达絽婀遍埢鏃堟煛鐏炶鍔滄慨锝嗗姍楠炴牗娼忛悙顒�顏�
 */
static unsigned short shellStringCopy(char *dest, char* src)
{
    unsigned short count = 0;
    while (*(src + count))
    {
        *(dest + count) = *(src + count);
        count++;
    }
    *(dest + count) = 0;
    return count;
}


/**
 * @brief shell闂佽瀛╃粙鎺椼�冮崼銉晞濞达絽婀遍埢鏃堟煕濞嗗浚妲烘俊鑼厴瀵爼宕奸浣割伓
 * 
 * @param dest 闂備胶鍎甸弲鈺呭窗濡ゅ懏鍋夐柨婵嗘祫閹峰嘲鈽夊▎妯荤暭濡炪倖鍨抽悞锔剧矙婢舵劖鏅搁柨鐕傛嫹
 * @param src 婵犵數濮嶉崟顐㈩潓闂佺儵濮嶉崨顖滅暥闂佹寧妫侀崕濠氥�呴敓锟�
 * @return unsigned short 闂備礁鎲￠悧鏇犵礊婵犲洤鍌ㄩ柕鍫濐槹閳锋劙鏌涢…鎴濇灈妞ゆ棑鎷�
 */
static unsigned short shellStringCompare(char* dest, char *src)
{
    unsigned short match = 0;
    unsigned short i = 0;

    while (*(dest +i) && *(src + i))
    {
        if (*(dest + i) != *(src +i))
        {
            break;
        }
        match ++;
        i++;
    }
    return match;
}


/**
 * @brief shell闂備礁鍚嬮崕鎶藉床閼艰翰浜归柛銉墮瀹告繈鏌熺�涙ê绗氬┑顔哄灲閺屾稑顫濋悙顒�顏�
 * 
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 * @return const char* 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濐槸鐟欙箓鏌ㄩ悤鍌涘
 */
static const char* shellGetCommandName(ShellCommand *command)
{
    static char buffer[9];
    for (unsigned char i = 0; i < 9; i++)
    {
        buffer[i] = '0';
    }
    if (command->attr.attrs.type <= SHELL_TYPE_CMD_FUNC)
    {
        return command->data.cmd.name;
    }
    else if (command->attr.attrs.type <= SHELL_TYPE_VAL)
    {
        return command->data.var.name;
    }
    else if (command->attr.attrs.type <= SHELL_TYPE_USER)
    {
        return command->data.user.name;
    }
    else
    {
        shellToHex(command->data.key.value, buffer);
        return buffer;
    }
}


/**
 * @brief shell闂備礁鍚嬮崕鎶藉床閼艰翰浜归柛銉墮瀹告繈鏌熺�涙ê绗氬┑顔哄灲閺岀喓鎷犻懠顒傤啈闂佸壊鍋撻幏锟�
 * 
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 * @return const char* 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濐槸缁犵敻鐓崶銊ㄥ闁绘鎷�
 */
static const char* shellGetCommandDesc(ShellCommand *command)
{
    if (command->attr.attrs.type <= SHELL_TYPE_CMD_FUNC)
    {
        return command->data.cmd.desc;
    }
    else if (command->attr.attrs.type <= SHELL_TYPE_VAL)
    {
        return command->data.var.desc;
    }
    else if (command->attr.attrs.type <= SHELL_TYPE_USER)
    {
        return command->data.user.desc;
    }
    else
    {
        return command->data.key.desc;
    }
}

/**
 * @brief shell 闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨瑰婵嬫煙鐎涙ê绗氬┑顔哄灲閺屸剝寰勫☉姘憋紵濠电偠顕滈幏锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param item 闂備礁鎲＄粙鎺楀垂濠靛绠柕鍫濐槸缁狙囨煥濞戞ê顏紒鎰舵嫹
 */
void shellListItem(Shell *shell, ShellCommand *item)
{
    short spaceLength;

    spaceLength = 22 - shellWriteString(shell, shellGetCommandName(item));
    spaceLength = (spaceLength > 0) ? spaceLength : 4;
    do {
        shellWriteByte(shell, ' ');
    } while (--spaceLength);
    if (item->attr.attrs.type <= SHELL_TYPE_CMD_FUNC)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_TYPE_CMD]);
    }
    else if (item->attr.attrs.type <= SHELL_TYPE_VAL)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_TYPE_VAR]);
    }
    else if (item->attr.attrs.type <= SHELL_TYPE_USER)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_TYPE_USER]);
    }
    else if (item->attr.attrs.type <= SHELL_TYPE_KEY)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_TYPE_KEY]);
    }
    else
    {
        shellWriteString(shell, shellText[SHELL_TEXT_TYPE_NONE]);
    }
#if SHELL_HELP_SHOW_PERMISSION == 1
    shellWriteString(shell, "  ");
    for (signed char i = 7; i >= 0; i--)
    {
        shellWriteByte(shell, item->attr.attrs.permission & (1 << i) ? 'x' : '-');
    }
#endif
    shellWriteString(shell, "  ");
    shellWriteCommandDesc(shell, shellGetCommandDesc(item));
    shellWriteString(shell, "\r\n");
}


/**
 * @brief shell闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨归惌妤併亜閺嶃劎鈽夐柍閿嬫閹泛鈽夊Ο鍨伃闂佽鏋奸崜婵堢矉閹烘鏅搁柨鐕傛嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellListCommand(Shell *shell)
{
    ShellCommand *base = (ShellCommand *)shell->commandList.base;
    shellWriteString(shell, shellText[SHELL_TEXT_CMD_LIST]);
    for (short i = 0; i < shell->commandList.count; i++)
    {
        if (base[i].attr.attrs.type <= SHELL_TYPE_CMD_FUNC
            && shellCheckPermission(shell, &base[i]) == 0)
        {
            shellListItem(shell, &base[i]);
        }
    }
}


/**
 * @brief shell闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨归惌妤佹叏濡ゅ瀚归梺闈╃秶閹凤拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellListVar(Shell *shell)
{
    ShellCommand *base = (ShellCommand *)shell->commandList.base;
    shellWriteString(shell, shellText[SHELL_TEXT_VAR_LIST]);
    for (short i = 0; i < shell->commandList.count; i++)
    {
        if (base[i].attr.attrs.type > SHELL_TYPE_CMD_FUNC
            && base[i].attr.attrs.type <= SHELL_TYPE_VAL
            && shellCheckPermission(shell, &base[i]) == 0)
        {
            shellListItem(shell, &base[i]);
        }
    }
}


/**
 * @brief shell闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨洪崑銊╂煏婵犲繒鐣遍柣鎿勬嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellListUser(Shell *shell)
{
    ShellCommand *base = (ShellCommand *)shell->commandList.base;
    shellWriteString(shell, shellText[SHELL_TEXT_USER_LIST]);
    for (short i = 0; i < shell->commandList.count; i++)
    {
        if (base[i].attr.attrs.type > SHELL_TYPE_VAL
            && base[i].attr.attrs.type <= SHELL_TYPE_USER
            && shellCheckPermission(shell, &base[i]) == 0)
        {
            shellListItem(shell, &base[i]);
        }
    }
}


/**
 * @brief shell闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨圭粻鏉款熆閼搁潧濮堥柡鍡嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellListKey(Shell *shell)
{
    ShellCommand *base = (ShellCommand *)shell->commandList.base;
    shellWriteString(shell, shellText[SHELL_TEXT_KEY_LIST]);
    for (short i = 0; i < shell->commandList.count; i++)
    {
        if (base[i].attr.attrs.type > SHELL_TYPE_USER
            && base[i].attr.attrs.type <= SHELL_TYPE_KEY
            && shellCheckPermission(shell, &base[i]) == 0)
        {
            shellListItem(shell, &base[i]);
        }
    }
}


/**
 * @brief shell闂備礁鎲＄敮妤呫�冮崨鏉戝惞妞ゆ挶鍨圭粻銉╂煥閻斿墎鐭欑�殿喖鐏氬鍕節閸愩劍顓煎┑鐐差嚟婵挳骞忛敓锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellListAll(Shell *shell)
{
#if SHELL_HELP_LIST_USER == 1
    shellListUser(shell);
#endif
    shellListCommand(shell);
#if SHELL_HELP_LIST_VAR == 1
    shellListVar(shell);
#endif
#if SHELL_HELP_LIST_KEY == 1
    shellListKey(shell);
#endif
}


/**
 * @brief shell闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓㈠弰鐎规洏鍔戦獮瀣箳閹炬壙鎴︽煟閻愬鈽夐柡鍫墴瀵娊鎮㈤悡搴ｎ唹闂佽法鍣﹂幏锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param length 闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓㈠弰婵﹤缍婂畷褰掝敊绾板崬鏁�
 */
void shellDeleteCommandLine(Shell *shell, unsigned char length)
{
    while (length--)
    {
        shellWriteString(shell, "\b \b");
    }
}


/**
 * @brief shell 婵犵數鍋為幐鎼佸箠閹版澘鐓橀柡宥庡幖瀹告繈鏌熺�涙ê绗氬┑顔哄灲閹泛鈽夊Ο鑲╁姰缂備線顤傞崣鍐ㄧ暦濡ゅ懏鏅搁柨鐕傛嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellClearCommandLine(Shell *shell)
{
    for (short i = shell->parser.length - shell->parser.cursor; i > 0; i--)
    {
        shellWriteByte(shell, ' ');
    }
    shellDeleteCommandLine(shell, shell->parser.length);
}


/**
 * @brief shell闂備礁婀辩划顖滄暜閹烘鐭楅柛鈩冾焽閳绘棃鏌ㄩ悢璇残撶紒瀣樀椤㈡棃宕熼鐔稿枙缂傚倷鐒﹂〃鎰不閹达箑鏄ョ�癸拷閸曨偆顦ф繝銏ｆ硾椤戝洭宕归悡骞熷綊鏁愰崨顖呫倝鏌ｉ妶蹇斿
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param data 闂佽瀛╃粙鎺椼�冮崼銉晞濞撴熬鎷风�殿噮鍋婇幃褔宕煎┑鍫涘亰
 */
void shellInsertByte(Shell *shell, char data)
{
    /* 闂備礁鎲＄敮鍥磹閺嶎厼钃熼柛銉ｅ妽缂嶅洭鏌熼幆褍鏆辩�殿喗濞婇弻鈩冩媴閸濆嫷鏆悗瑙勬尫缁舵艾顕ｆ导鎼晬婵﹩鍘奸崜銊╂煛婢跺棙娅呮繛宸弮瀵娊鏁撻敓锟� */
    if (shell->parser.length >= shell->parser.bufferSize - 1)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_CMD_TOO_LONG]);
        shellWriteCommandLine(shell);
        shellWriteString(shell, shell->parser.buffer);
        return;
    }

    /* 闂備礁婀辩划顖滄暜閹烘鐭楅柛鈩冪☉閺嬩線鏌ｅΔ锟介悧鍡欑矈閿燂拷 */
    if (shell->parser.cursor == shell->parser.length)
    {
        shell->parser.buffer[shell->parser.length++] = data;
        shell->parser.buffer[shell->parser.length] = 0;
        shell->parser.cursor++;
        shellWriteByte(shell, data);
    }
    else if (shell->parser.cursor < shell->parser.length)
    {
        for (short i = shell->parser.length - shell->parser.cursor; i > 0; i--)
        {
            shell->parser.buffer[shell->parser.cursor + i] = 
                shell->parser.buffer[shell->parser.cursor + i - 1];
        }
        shell->parser.buffer[shell->parser.cursor++] = data;
        shell->parser.buffer[++shell->parser.length] = 0;
        for (short i = shell->parser.cursor - 1; i < shell->parser.length; i++)
        {
            shellWriteByte(shell, shell->parser.buffer[i]);
        }
        for (short i = shell->parser.length - shell->parser.cursor; i > 0; i--)
        {
            shellWriteByte(shell, '\b');
        }
    }
}


/**
 * @brief shell 闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓熸崳闁跨喕濮ょ粙鎺椼�冩径瀣╃箚闁跨噦鎷�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param direction 闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓㈠弰鐎殿噮鍓熼幃鐑芥偋閸繐鎷烽敓锟� {@code 1}闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓㈠弰鐎规洘顨嗗鍕拷锝庡墮閻忛亶姊洪崨濠冨碍缂佸鎸抽幃楦款槾缂侇噮鍙冮弫鎾绘晸閿燂拷 {@code -1}闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崨顓㈠弰鐎规洘顨嗗鍕拷锝庡墮閻忚鲸绻濋姀锝嗙【閻庢凹鍓熼幃楦款槾缂侇噮鍙冮弫鎾绘晸閿燂拷
 */
void shellDeleteByte(Shell *shell, signed char direction)
{
    char offset = (direction == -1) ? 1 : 0;

    if ((shell->parser.cursor == 0 && direction == 1)
        || (shell->parser.cursor == shell->parser.length && direction == -1))
    {
        return;
    }
    if (shell->parser.cursor == shell->parser.length && direction == 1)
    {
        shell->parser.cursor--;
        shell->parser.length--;
        shell->parser.buffer[shell->parser.length] = 0;
        shellDeleteCommandLine(shell, 1);
    }
    else
    {
        for (short i = offset; i < shell->parser.length - shell->parser.cursor; i++)
        {
            shell->parser.buffer[shell->parser.cursor + i - 1] = 
                shell->parser.buffer[shell->parser.cursor + i];
        }
        shell->parser.length--;
        if (!offset)
        {
            shell->parser.cursor--;
            shellWriteByte(shell, '\b');
        }
        shell->parser.buffer[shell->parser.length] = 0;
        for (short i = shell->parser.cursor; i < shell->parser.length; i++)
        {
            shellWriteByte(shell, shell->parser.buffer[i]);
        }
        shellWriteByte(shell, ' ');
        for (short i = shell->parser.length - shell->parser.cursor + 1; i > 0; i--)
        {
            shellWriteByte(shell, '\b');
        }
    }
}


/**
 * @brief shell 闂佽崵鍠愰悷杈╁緤妤ｅ啯鍊靛ù鐘差儏閻鏌涚仦鍓р檨婵炲牞鎷�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
static void shellParserParam(Shell *shell)
{
    unsigned char quotes = 0;
    unsigned char record = 1;

    shell->parser.paramCount = 0;
	//printf("%s %d %d %d\n",__func__,__LINE__,shell->parser.length,shell->parser.paramCount);
    for (unsigned short i = 0; i < shell->parser.length; i++)
    {
		//printf("%s %d %C \n",__func__,__LINE__,shell->parser.buffer[i]);
        if (quotes != 0
            || (shell->parser.buffer[i] != ' '
                && shell->parser.buffer[i] != 0))
        {
            if (record == 1)
            {
                shell->parser.param[shell->parser.paramCount ++] = 
                    &(shell->parser.buffer[i]);
                record = 0;
            }
            if (shell->parser.buffer[i] == '\\'
                && shell->parser.buffer[i + 1] != 0)
            {
                i++;
            }
        }
        else
        {
            shell->parser.buffer[i] = 0;
            record = 1;
        }
    }
	//printf("%s %d %d %d\n",__func__,__LINE__,shell->parser.length,shell->parser.paramCount);
}


/**
 * @brief shell闂備礁鎲￠敋妞ゎ厼鍢查埢宥嗘償閿濆繑瀚规繛鎴炵懐濞堟ɑ銇勯幋婊呭妽缂佸顦甸獮鎾诲箳閹寸儐妫熼梻浣芥〃閻掞箑顭垮锟芥俊鐢稿籍閸屾瑧鍓ㄩ梺鍛婃寙閸曨厽娈搁梻浣告啞閻熴儵鎳熼婊勵潟闁冲搫鎳庨惌妤呮煥閻曞倹瀚�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
static void shellRemoveParamQuotes(Shell *shell)
{
    unsigned short paramLength;
	//printf("%s %d %d \n",__func__,__LINE__,shell->parser.paramCount);
    for (unsigned short i = 0; i < shell->parser.paramCount; i++)
    {
        if (shell->parser.param[i][0] == '\"')
        {
            shell->parser.param[i][0] = 0;
            shell->parser.param[i] = &shell->parser.param[i][1];
        }
        paramLength = strlen(shell->parser.param[i]);
        if (shell->parser.param[i][paramLength - 1] == '\"')
        {
            shell->parser.param[i][paramLength - 1] = 0;
        }
    }
}


/**
 * @brief shell闂備礁鎲￠悧鏇犵礊婵犲洤鍌ㄩ柕鍫濐槸瀹告繈鏌熺�涙ê绗氬┑顕嗘嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param cmd 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 * @param base 闂備礁鎲￠悧鏇犵礊婵犲洤鍌ㄩ柕鍫濐槸瀹告繈鏌熺�涙ê绗氬┑顔哄灲閹泛鈽夐弽褍顬堥梺缁樻閹风兘姊虹粔鍡楃У鐎氾拷
 * @param compareLength 闂備礁鎲￠悧鏇犵礊婵犲洤鍌ㄩ柕鍫濇祫閹峰嘲鈽夊▎妯荤暭濡炪倖鍨抽悞锔剧矙婢舵劕钃熼柕澶涢檮閻︼綁鏌熼懝鐗堝涧闁瑰嚖鎷�
 * @return ShellCommand* 闂備礁鎲￠悧鏇犵礊婵犲洤鍌ㄩ柕鍫濐槸缁�鍡涙煙濞堝灝鏋熼柣锝変憾閺屾稑鈽夊▎蹇曚喊闂佺顔愰幏锟�
 */
ShellCommand* shellSeekCommand(Shell *shell,
                               const char *cmd,
                               ShellCommand *base,
                               unsigned short compareLength)
{
    const char *name;
    unsigned short count = shell->commandList.count -
        ((int)base - (int)shell->commandList.base) / sizeof(ShellCommand);
    for (unsigned short i = 0; i < count; i++)
    {
        if (base[i].attr.attrs.type == SHELL_TYPE_KEY
            || shellCheckPermission(shell, &base[i]) != 0)
        {
            continue;
        }
        name = shellGetCommandName(&base[i]);
        if (!compareLength)
        {
            if (strcmp(cmd, name) == 0)
            {
                return &base[i];
            }
        }
        else
        {
            if (strncmp(cmd, name, compareLength) == 0)
            {
                return &base[i];
            }
        }
    }
    return NULL;
}


/**
 * @brief shell 闂備礁鍚嬮崕鎶藉床閼艰翰浜归柛銉墮閻鎱ㄥΔ瀣闂侀潻绲惧钘夌暦閹扮増鏅搁柨鐕傛嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 * @return int 闂備礁鎲￠悷锕傛晝閵忋倕闂繛宸簻绾惧綊鏌ㄩ悤鍌涘
 */
int shellGetVarValue(Shell *shell, ShellCommand *command)
{
    int value = 0;
    switch (command->attr.attrs.type)
    {
    case SHELL_TYPE_VAR_INT:
        value = *((int *)(command->data.var.value));
        break;
    case SHELL_TYPE_VAR_SHORT:
        value = *((short *)(command->data.var.value));
        break;
    case SHELL_TYPE_VAR_CHAR:
        value = *((char *)(command->data.var.value));
        break;
    case SHELL_TYPE_VAR_POINT:
    case SHELL_TYPE_VAL:
        value = (int)(command->data.var.value);
        break;
    default:
        break;
    }
    return value;
}


/**
 * @brief shell闂佽崵濮崇粈浣规櫠娴犲鍋柛鈩冪☉閻鎱ㄥΔ瀣闂侀潻绲惧钘夌暦閹扮増鏅搁柨鐕傛嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 * @param value 闂備胶顭堥…顓㈠箯閿燂拷
 */
void shellSetVarValue(Shell *shell, ShellCommand *command, int value)
{
    switch (command->attr.attrs.type)
    {
    case SHELL_TYPE_VAR_INT:
        *((int *)(command->data.var.value)) = value;
        break;
    case SHELL_TYPE_VAR_SHORT:
        *((short *)(command->data.var.value)) = value;
        break;
    case SHELL_TYPE_VAR_CHAR:
        *((char *)(command->data.var.value)) = value;
        break;
    case SHELL_TYPE_VAR_POINT:
        shellWriteString(shell, shellText[SHELL_TEXT_POINT_CANNOT_MODIFY]);
        break;
    case SHELL_TYPE_VAL:
        shellWriteString(shell, shellText[SHELL_TEXT_VAL_CANNOT_MODIFY]);
        break;
    default:
        break;
    }
    shellShowVar(shell, command);
}


/**
 * @brief shell闂備礁鎲￠悷锕傛晝閵忋倕闂柣鎴炆戠紞鍥煙閹冩毐婵綇鎷�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 */
static void shellShowVar(Shell *shell, ShellCommand *command)
{
    char buffer[12] = "00000000000";
    int value = shellGetVarValue(shell, command);
    shellWriteString(shell, command->data.var.name);
    shellWriteString(shell, " = ");
    shellWriteString(shell, &buffer[11 - shellToDec(value, buffer)]);
    shellWriteString(shell, ", 0x");
    for (short i = 0; i < 11; i++)
    {
        buffer[i] = '0';
    }
    shellToHex(value, buffer);
    shellWriteString(shell, buffer);
    shellWriteString(shell, "\r\n");
}


/**
 * @brief shell闂佽崵濮崇粈浣规櫠娴犲鍋柛鈩冪☉閻鎱ㄥΔ瀣闂侀潻缍囬幏锟�
 * 
 * @param name 闂備礁鎲￠悷锕傛晝閵忋倕闂繛宸簻鐟欙箓鏌ㄩ悤鍌涘
 * @param value 闂備礁鎲￠悷锕傛晝閵忋倕闂繛宸簻绾惧綊鏌ㄩ悤鍌涘
 * @return int 闂佸搫顦弲婊堝蓟閵娿儍娲冀椤撶偟鐓戝┑顔筋殣閹风兘鏌涢敐鍡樸仢鐎规洘鍔欓弫鎾绘晸閿燂拷
 */
int shellSetVar(char *name, int value)
{
    Shell *shell = shellGetCurrent();
    if (shell == NULL)
    {
        return 0;
    }
    ShellCommand *command = shellSeekCommand(shell,
                                             name,
                                             shell->commandList.base,
                                             0);
    if (!command)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_VAR_NOT_FOUND]);
        return 0;
    }
    if (command->attr.attrs.type < SHELL_TYPE_VAR_INT
        || command->attr.attrs.type > SHELL_TYPE_VAL)
    {
        shellWriteString(shell, name);
        shellWriteString(shell, shellText[SHELL_TEXT_NOT_VAR]);
        return 0;
    }
    shellSetVarValue(shell, command, value);
    return shellGetVarValue(shell, command);
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
setVar, shellSetVar, set var);


/**
 * @brief shell闂佸搫顦弲婊堝礉濮楋拷閵嗕線骞嬮敃锟藉婵嬫煙鐎涙ê绗氬┑顕嗘嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param command 闂備礁鎲＄粙鎺楀垂濠靛绠柨鐕傛嫹
 */
static void shellRunCommand(Shell *shell, ShellCommand *command)
{
    int returnValue;
    shell->status.isActive = 1;
    if (command->attr.attrs.type == SHELL_TYPE_CMD_MAIN)
    {
        shellRemoveParamQuotes(shell);
        returnValue = command->data.cmd.function(shell->parser.paramCount,shell->parser.param);
        if (!command->attr.attrs.disableReturn)
        {
            shellWirteReturnValue(shell, returnValue);
        }
    }
    else if (command->attr.attrs.type == SHELL_TYPE_CMD_FUNC)
    {
        returnValue = shellExtRun(shell,
                                  command->data.cmd.function,
                                  shell->parser.paramCount,
                                  shell->parser.param);
        if (!command->attr.attrs.disableReturn)
        {
            shellWirteReturnValue(shell, returnValue);
        }
    }
    else if (command->attr.attrs.type >= SHELL_TYPE_VAR_INT
        && command->attr.attrs.type <= SHELL_TYPE_VAL)
    {
        shellShowVar(shell, command);
    }
    else if (command->attr.attrs.type == SHELL_TYPE_USER)
    {
        shellSetUser(shell, command);
    }
    shell->status.isActive = 0;
}


/**
 * @brief shell闂備礁鎼粙鍕崲濠靛鍋樻繛鍡樺煀閹风兘妫冨☉娆樻￥闂佺粯鐗幏锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
static void shellCheckPassword(Shell *shell)
{
    if (strcmp(shell->parser.buffer, shell->info.user->data.user.password) == 0)
    {
        shell->status.isChecked = 1;
        shellWriteString(shell, shellText[SHELL_TEXT_INFO]);
    }
    else
    {
        shellWriteString(shell, shellText[SHELL_TEXT_PASSWORD_ERROR]);
    }
    shell->parser.length = 0;
    shell->parser.cursor = 0;
}


/**
 * @brief shell闂佽崵濮崇粈浣规櫠娴犲鍋柛鈩冪♁閸嬨劑鏌曟繝蹇曠暠闁绘搫鎷�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param user 闂備胶鍎甸崑鎾诲礉瀹ュ鏄ラ柨鐕傛嫹
 */
static void shellSetUser(Shell *shell, const ShellCommand *user)
{
    shell->info.user = user;
    shell->status.isChecked = 
        ((user->data.user.password && strlen(user->data.user.password) != 0)
            && (shell->parser.paramCount == 1
                || strcmp(user->data.user.password, shell->parser.param[1]) != 0))
         ? 0 : 1;
        
    shellWriteString(shell, shellText[SHELL_TEXT_CLEAR_CONSOLE]);
    if (shell->status.isChecked)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_INFO]);
    }
}


/**
 * @brief shell闂備礁鎲￠崝鏍偡閵娧勫床闁硅揪绠戦悙濠囨煟閹邦垼鍤嬮柟椋庡厴閺佹捇鏁撻敓锟�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param value 闂佸搫顦弲婊堝蓟閵娿儍娲冀椤撶偟锛欓梺璺ㄥ櫐閹凤拷
 */
static void shellWirteReturnValue(Shell *shell, int value)
{
    char buffer[12] = "00000000000";
    shellWriteString(shell, "Return: ");
    shellWriteString(shell, &buffer[11 - shellToDec(value, buffer)]);
    shellWriteString(shell, ", 0x");
    for (short i = 0; i < 11; i++)
    {
        buffer[i] = '0';
    }
    shellToHex(value, buffer);
    shellWriteString(shell, buffer);
    shellWriteString(shell, "\r\n");
}


/**
 * @brief shell闂備礁鎲￠敋婵☆偅顨夐妵鎰板礈娴ｇ懓顎涢梺鍝勵槼濞夋洜绮旈幐搴ｇ闁挎繂鐗滈崵娆忊攽椤曞棙瀚�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
static void shellHistoryAdd(Shell *shell)
{
    shell->history.offset = 0;
    if (shell->history.number > 0
        && strcmp(shell->history.item[(shell->history.record == 0 ? 
                SHELL_HISTORY_MAX_NUMBER : shell->history.record) - 1],
                shell->parser.buffer) == 0)
    {
        return;
    }
    if (shellStringCopy(shell->history.item[shell->history.record],
                        shell->parser.buffer) != 0)
    {
        shell->history.record++;
    }
    if (++shell->history.number > SHELL_HISTORY_MAX_NUMBER)
    {
        shell->history.number = SHELL_HISTORY_MAX_NUMBER;
    }
    if (shell->history.record >= SHELL_HISTORY_MAX_NUMBER)
    {
        shell->history.record = 0;
    }
}


/**
 * @brief shell闂備礁鎲￠敋婵☆偅顨夐妵鎰板礈娴ｇ懓顎涢梺鍝勵槼濞夋洜绮旈崸妤佺厸閻忕偠顕ч崝婊冾熆閻у憡瀚�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param dir 闂備礁鎼崐濠氬箠閹捐绠栭柨鐕傛嫹 {@code <0}闁诲酣娼ф惔婊堝箯瀹勬壋妲堥柟鍓ь劜閺�濠氭煟閹垮嫮绡�妤犵偛绉归弫鎾绘晸閿燂拷 {@code >0}闁诲酣娼ф惔婊堝箯瀹勬壋妲堥柟鐐墯閸庢劙鏌ｉ幙鍕瘈妤犵偛绉归弫鎾绘晸閿燂拷
 */
static void shellHistory(Shell *shell, signed char dir)
{
    if (dir > 0)
    {
        if (shell->history.offset-- <= 
            -((shell->history.number > shell->history.record) ?
                shell->history.number : shell->history.record))
        {
            shell->history.offset = -((shell->history.number > shell->history.record)
                                    ? shell->history.number : shell->history.record);
        }
    }
    else if (dir < 0)
    {
        if (++shell->history.offset > 0)
        {
            shell->history.offset = 0;
            return;
        }
    }
    else
    {
        return;
    }
    shellClearCommandLine(shell);
    if (shell->history.offset == 0)
    {
        shell->parser.cursor = shell->parser.length = 0;
    }
    else
    {
        if ((shell->parser.length = shellStringCopy(shell->parser.buffer,
                shell->history.item[(shell->history.record + SHELL_HISTORY_MAX_NUMBER
                    + shell->history.offset) % SHELL_HISTORY_MAX_NUMBER])) == 0)
        {
            return;
        }
        shell->parser.cursor = shell->parser.length;
        shellWriteString(shell, shell->parser.buffer);
    }
    
}


/**
 * @brief shell 闂佹眹鍩勯崹閬嶆偤閺囶澁缍栧璺侯儐缂嶅洭鏌熼幆褍鏆辩�殿噯鎷�
 * 
 * @param shell shell 闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param data 闂佸搫顦悧濠囧箰閹间礁鐭楅柛鈩冪懕閹峰嘲鈽夊▎妯荤暭濡炪倖鍩婇幏锟�
 */
void shellNormalInput(Shell *shell, char data)
{
    shell->status.tabFlag = 0;
    shellInsertByte(shell, data);
}


/**
 * @brief shell濠电偞鍨堕幐鎼佹晝閵夆晛钃熼柣鏃傚帶鐟欙箓鏌熸潏楣冩闁哄棙鐩鍫曞醇閻旂纰嶉梺鍛婄懕閹凤拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellUp(Shell *shell)
{
    shellHistory(shell, 1);
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x1B5B4100, shellUp, up);


/**
 * @brief shell濠电偞鍨堕幐鎼侇敄閸℃稑钃熼柣鏃傚帶鐟欙箓鏌熸潏楣冩闁哄棙鐩鍫曞醇閻旂纰嶉梺鍛婄懕閹凤拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellDown(Shell *shell)
{
    shellHistory(shell, -1);
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x1B5B4200, shellDown, down);


/**
 * @brief shell闂備礁鎲￠悷銊モ枍閺囥垹钃熼柣鏃傚帶鐟欙箓鏌熸潏楣冩闁哄棙鐩鍫曞醇閻旂纰嶉梺鍛婄懕閹凤拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellRight(Shell *shell)
{
    if (shell->parser.cursor < shell->parser.length)
    {
        shellWriteByte(shell, shell->parser.buffer[shell->parser.cursor++]);
    }
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x1B5B4300, shellRight, right);


/**
 * @brief shell闁诲骸缍婂濠氭⒔閸曨垰钃熼柣鏃傚帶鐟欙箓鏌熸潏楣冩闁哄棙鐩鍫曞醇閻旂纰嶉梺鍛婄懕閹凤拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellLeft(Shell *shell)
{
    if (shell->parser.cursor > 0)
    {
        shellWriteByte(shell, '\b');
        shell->parser.cursor--;
    }
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x1B5B4400, shellLeft, left);


/**
 * @brief shell Tab闂備礁婀遍…鍫ニ囬悽绋挎瀬妞ゆ挾鍠庣欢鐐烘煕閺囥劌骞楅柛濠忔嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellTab(Shell *shell)
{
    unsigned short maxMatch = shell->parser.bufferSize;
    unsigned short lastMatchIndex = 0;
    unsigned short matchNum = 0;
    unsigned short length;

    if (shell->parser.length == 0)
    {
        shellListAll(shell);
        shellWriteCommandLine(shell);
    }
    else if (shell->parser.length > 0)
    {
        shell->parser.buffer[shell->parser.length] = 0;
        ShellCommand *base = (ShellCommand *)shell->commandList.base;
        for (short i = 0; i < shell->commandList.count; i++)
        {
            if (shellCheckPermission(shell, &base[i]) == 0
                && shellStringCompare(shell->parser.buffer,
                                   (char *)shellGetCommandName(&base[i]))
                        == shell->parser.length)
            {
                if (matchNum != 0)
                {
                    if (matchNum == 1)
                    {
                        shellWriteString(shell, "\r\n");
                    }
                    shellListItem(shell, &base[lastMatchIndex]);
                    length = 
                        shellStringCompare((char *)shellGetCommandName(&base[lastMatchIndex]),
                                           (char *)shellGetCommandName(&base[i]));
                    maxMatch = (maxMatch > length) ? length : maxMatch;
                }
                lastMatchIndex = i;
                matchNum++;
            }
        }
        if (matchNum == 0)
        {
            return;
        }
        if (matchNum == 1)
        {
            shellClearCommandLine(shell);
        }
        if (matchNum != 0)
        {
            shell->parser.length = 
                shellStringCopy(shell->parser.buffer,
                                (char *)shellGetCommandName(&base[lastMatchIndex]));
        }
        if (matchNum > 1)
        {
            shellListItem(shell, &base[lastMatchIndex]);
            shellWriteCommandLine(shell);
            shell->parser.length = maxMatch;
        }
        shell->parser.buffer[shell->parser.length] = 0;
        shell->parser.cursor = shell->parser.length;
        shellWriteString(shell, shell->parser.buffer);
    }

    if (SHELL_GET_TICK())
    {
        if (matchNum == 1
            && shell->status.tabFlag
            && SHELL_GET_TICK() - shell->info.activeTime < SHELL_DOUBLE_CLICK_TIME)
        {
            shellClearCommandLine(shell);
            for (short i = shell->parser.length; i >= 0; i--)
            {
                shell->parser.buffer[i + 5] = shell->parser.buffer[i];
            }
            shellStringCopy(shell->parser.buffer, "help");
            shell->parser.buffer[4] = ' ';
            shell->parser.length += 5;
            shell->parser.cursor = shell->parser.length;
            shellWriteString(shell, shell->parser.buffer);
        }
        else
        {
            shell->status.tabFlag = 1;
        }
    }
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x09000000, shellTab, tab);


/**
 * @brief shell 闂傚倷绶￠崑鎾诲箯閻戣姤鐓涚�广儱绻戠�氾拷
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellBackspace(Shell *shell)
{
    shellDeleteByte(shell, 1);
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x08000000, shellBackspace, backspace);


/**
 * @brief shell 闂備礁鎲＄敮鐐寸箾閿熻姤绻涢崪鍐╁
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellDelete(Shell *shell)
{
    shellDeleteByte(shell, -1);
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x7F000000, shellDelete, delete);
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x1B5B337E, shellDelete, delete);

/**
 * @brief shell 闂備焦鎮堕崕鎶藉磻閻樻祴鏀﹂柛娑卞枛缁剁偤鏌涢弴銊ュ箺闁稿鎷�
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 */
void shellEnter(Shell *shell)
{
    if (shell->parser.length == 0)
    {
        shellWriteCommandLine(shell);
        return;
    }

    shell->parser.buffer[shell->parser.length] = 0;

    if (shell->status.isChecked)
    {
        shellHistoryAdd(shell);
        shellParserParam(shell);
        shell->parser.length = shell->parser.cursor = 0;
        if (shell->parser.paramCount == 0)
        {
            shellWriteCommandLine(shell);
            return;
        }
        shellWriteString(shell, "\r\n");

        ShellCommand *command = shellSeekCommand(shell,
                                                 shell->parser.param[0],
                                                 shell->commandList.base,
                                                 0);
        if (command != NULL)
        {
            shellRunCommand(shell, command);
        }
        else
        {
            shellWriteString(shell, shellText[SHELL_TEXT_CMD_NOT_FOUND]);
        }
    }
    else
    {
        shellCheckPassword(shell);
    }
    shellWriteCommandLine(shell);
}
#if SHELL_ENTER_LF == 1
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x0A000000, shellEnter, enter);
#endif
#if SHELL_ENTER_CR == 1
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x0D000000, shellEnter, enter);
#endif
#if SHELL_ENTER_CRLF == 1
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0)|SHELL_CMD_ENABLE_UNCHECKED,
0x0D0A0000, shellEnter, enter);
#endif


/**
 * @brief shell help
 * 
 * @param argc 闂備礁鎲￠悷銉╁磹瑜版帒姹查柣鏂挎憸閳绘梹銇勮箛鎾村櫤婵炲牞鎷�
 * @param argv 闂備礁鎲￠悷銉╁磹瑜版帒姹查柨鐕傛嫹
 */
void shellHelp(int argc, char *argv[])
{
    Shell *shell = shellGetCurrent();
    SHELL_ASSERT(shell, return);
    if (argc == 1)
    {
        shellListAll(shell);
    }
    else if (argc > 1)
    {
        ShellCommand *command = shellSeekCommand(shell,
                                                 argv[1],
                                                 shell->commandList.base,
                                                 0);
        shellWriteString(shell, shellText[SHELL_TEXT_HELP_HEADER]);
        shellWriteString(shell, shellGetCommandName(command));
        shellWriteString(shell, "\r\n");
        shellWriteString(shell, shellGetCommandDesc(command));
        shellWriteString(shell, "\r\n");
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
help, shellHelp, show command info\r\nhelp [cmd]);

/**
 * @brief shell 闂佸搫顦悧濠囧箰閹间礁鐭楅柛鈩兠欢鐐烘煕閺囥劌骞楅柛濠忔嫹
 * 
 * @param shell shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�
 * @param data 闂佸搫顦悧濠囧箰閹间礁鐭楅柛鈩冪☉閺嬩線鏌ｅΔ锟介悧鍡欑矈閿燂拷
 */
void shellHandler(Shell *shell, char data)
{
    SHELL_ASSERT(data, return);


#if SHELL_LOCK_TIMEOUT > 0
    if (shell->info.user->data.user.password
        && strlen(shell->info.user->data.user.password) != 0
        && SHELL_GET_TICK())
    {
        if (SHELL_GET_TICK() - shell->info.activeTime > SHELL_LOCK_TIMEOUT)
        {
            shell->status.isChecked = 0;
        }
    }
#endif


    char keyByteOffset = 24;
    int keyFilter = 0x00000000;
    if ((shell->parser.keyValue & 0x0000FF00) != 0x00000000)
    {
        keyByteOffset = 0;
        keyFilter = 0xFFFFFF00;
    }
    else if ((shell->parser.keyValue & 0x00FF0000) != 0x00000000)
    {
        keyByteOffset = 8;
        keyFilter = 0xFFFF0000;
    }
    else if ((shell->parser.keyValue & 0xFF000000) != 0x00000000)
    {
        keyByteOffset = 16;
        keyFilter = 0xFF000000;
    }

   
    ShellCommand *base = (ShellCommand *)shell->commandList.base;
	
    for (short i = 0; i < shell->commandList.count; i++)
    {
        /* 闂備礁鎲＄敮鍥磹閺嶎厼钃熼柛銉墮閸欏﹥銇勯弽銊ь暡闁稿骸锕弻锟犲礋闂堟稓浠柣搴＄仛缁捇寮婚妸鈺婃晩闁兼祴鏅滃В搴㈢箾閹寸偞鐓ユい锔诲灦閹崇喖鎮㈤崜鍙夘啍閻庣櫢鎷烽柛鎰典簼椤秹姊洪崫鍕垫Ц闁哥喐娼欓‖濠囨晸閿燂拷 */
        if (base[i].attr.attrs.type == SHELL_TYPE_KEY
            && shellCheckPermission(shell, &(base[i])) == 0)
        {
            /* 闂佽娴烽弫鎼併�佹繝鍕偨妞ゆ挶鍨圭粈鍌炴煏婢诡垰瀚弳鐘绘煟鎼淬垻鈾佹い銊ヮ槹娣囧﹪顢橀姀鐘盒曢柣蹇曞仦閸庢娊鎯佹繝姘拺婵炲棙鍎冲▍宥夋煛娓氬洤寮�规洘鍔欓幊婵嬪箥椤旂虎鍟嬮梺鑽ゅ仦缁嬫垿鎳熼婊�鐒婃い鏇嫹闁哄苯锕弫鎾绘晸閿燂拷 */
            if ((base[i].data.key.value & keyFilter) == shell->parser.keyValue
                && (base[i].data.key.value & (0xFF << keyByteOffset))
                    == (data << keyByteOffset))
            {
                shell->parser.keyValue |= data << keyByteOffset;
                data = 0x00;
                if (keyByteOffset == 0 
                    || (base[i].data.key.value & (0xFF << (keyByteOffset - 8)))
                        == 0x00000000)
                {
                    if (base[i].data.key.function)
                    {
                        base[i].data.key.function(shell);
						//printf("%p \n",base[i].data.key.function);
                    }
                    shell->parser.keyValue = 0x00000000;
                    break;
                }
            }
        }
    }

    if (data != 0x00)
    {
        shellNormalInput(shell, data);
    }

    if (SHELL_GET_TICK())
    {
        shell->info.activeTime = SHELL_GET_TICK();
    }
}


/**
 * @brief shell 濠电偛顕慨楣冾敋瑜庨幈銊╂晸閿燂拷
 * 
 * @param param 闂備礁鎲￠悷銉╁磹瑜版帒姹查柨鐕傛嫹(shell闂佽娴烽弫鎼併�佹繝鍥ㄥ瘶闁跨噦鎷�)
 * 
 */
void shellTask(void *param)
{
    Shell *shell = (Shell *)param;
#ifdef WIN32
    char data[128];
#if SHELL_TASK_WHILE == 1
    //  while(1)
    {
#endif
        int c = 0, i ;
        if (shell->read && (c = shell->read(data)) != 0)
        {
            for(i = 0 ; i < c ; i ++)
                shellHandler(shell, data[i]);
        }
#if SHELL_TASK_WHILE == 1
    }
#endif
#else
    char data;
#if SHELL_TASK_WHILE == 1
    //  while(1)
    {
#endif
        if (shell->read && shell->read(&data) == 0)
        {
            shellHandler(shell, data);
        }
#if SHELL_TASK_WHILE == 1
    }
#endif
#endif
}


/**
 * @brief shell 闂佸搫顦悧濠囧箰閹间礁鍚规い鎾卞灪閸嬨劑鏌曟繝蹇曠暠闁绘挻娲熼弻娑㈠箳閹垮啯鐣介梺闈涙４閹凤拷(shell闂佽崵濮撮鍛村疮娴兼潙鏋侀柨鐕傛嫹)
 */
void shellUsers(void)
{
    Shell *shell = shellGetCurrent();
    if (shell)
    {
        shellListUser(shell);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
users, shellUsers, list all user);


/**
 * @brief shell 闂佸搫顦悧濠囧箰閹间礁鍚规い鎾卞灩瀹告繈鏌熺�涙ê绗氬┑顔哄灲閺屾盯骞掗幙鍐╃暯闂侀潧妫撮幏锟�(shell闂佽崵濮撮鍛村疮娴兼潙鏋侀柨鐕傛嫹)
 */
void shellCmds(void)
{
    Shell *shell = shellGetCurrent();
    if (shell)
    {
        shellListCommand(shell);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
cmds, shellCmds, list all cmd);


/**
 * @brief shell 闂佸搫顦悧濠囧箰閹间礁鍚规い鎾卞灩閻鎱ㄥΔ瀣闂侀潻绲惧钘夌暦濮橆儵鏃堝礋閳哄瀚归柨鐕傛嫹(shell闂佽崵濮撮鍛村疮娴兼潙鏋侀柨鐕傛嫹)
 */
void shellVars(void)
{
    Shell *shell = shellGetCurrent();
    if (shell)
    {
        shellListVar(shell);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
vars, shellVars, list all var);


/**
 * @brief shell 闂佸搫顦悧濠囧箰閹间礁鍚规い鎾卞灩缁犳澘顭块懜闈涘闁哄棙鐩弻娑㈠箳閹垮啯鐣介梺闈涙４閹凤拷(shell闂佽崵濮撮鍛村疮娴兼潙鏋侀柨鐕傛嫹)
 */
void shellKeys(void)
{
    Shell *shell = shellGetCurrent();
    if (shell)
    {
        shellListKey(shell);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
keys, shellKeys, list all key);


/**
 * @brief shell 婵犵數鍋為幐鎼佸箠閹版澘鐓橀柡宥庡幖缁犲磭鎲稿澶婃槬婵°倕鎳庨惌妤呮煥閻曞倹瀚�(shell闂佽崵濮撮鍛村疮娴兼潙鏋侀柨鐕傛嫹)
 */
void shellClear(void)
{
    Shell *shell = shellGetCurrent();
    if (shell)
    {
        shellWriteString(shell, shellText[SHELL_TEXT_CLEAR_CONSOLE]);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_DISABLE_RETURN,
clear, shellClear, clear console);

