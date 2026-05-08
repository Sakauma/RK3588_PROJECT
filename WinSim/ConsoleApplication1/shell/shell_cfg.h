/**
 * @file shell_cfg.h
 * @author Letter (nevermindzzt@gmail.com)
 * @brief shell config
 * @version 3.0.0
 * @date 2019-12-31
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#ifndef __SHELL_CFG_H__
#define __SHELL_CFG_H__


/**
 * @brief 鏄惁浣跨敤榛樿shell浠诲姟while寰幆锛屼娇鑳藉畯`SHELL_USING_TASK`鍚庢瀹忔湁鎰忎箟
 *        浣胯兘姝ゅ畯锛屽垯`shellTask()`鍑芥暟浼氫竴鐩村惊鐜鍙栬緭鍏ワ紝涓�鑸娇鐢ㄦ搷浣滅郴缁熷缓绔媠hell
 *        浠诲姟鏃朵娇鑳芥瀹忥紝鍏抽棴姝ゅ畯鐨勬儏鍐典笅锛屼竴鑸�傜敤浜庢棤鎿嶄綔绯荤粺锛屽湪涓诲惊鐜腑璋冪敤`shellTask()`
 */
#define     SHELL_TASK_WHILE            1
/**
 * @brief 鏄惁浣跨敤鍛戒护瀵煎嚭鏂瑰紡
 *        浣胯兘姝ゅ畯鍚庯紝鍙互浣跨敤`SHELL_EXPORT_CMD()`绛夊鍑哄懡浠�
 *        瀹氫箟shell鍛戒护锛屽叧闂瀹忕殑鎯呭喌涓嬶紝闇�瑕佷娇鐢ㄥ懡浠よ〃鐨勬柟寮�
 */
#define     SHELL_USING_CMD_EXPORT      0

/**
 * @brief 鏄惁鍦ㄨ緭鍑哄懡浠ゅ垪琛ㄤ腑鍒楀嚭鐢ㄦ埛
 */
#define     SHELL_HELP_LIST_USER        0

/**
 * @brief 鏄惁鍦ㄨ緭鍑哄懡浠ゅ垪琛ㄤ腑鍒楀嚭鍙橀噺
 */
#define     SHELL_HELP_LIST_VAR         0

/**
 * @brief 鏄惁鍦ㄨ緭鍑哄懡浠ゅ垪琛ㄤ腑鍒楀嚭鎸夐敭
 */
#define     SHELL_HELP_LIST_KEY         0

/**
 * @brief 鏄惁鍦ㄨ緭鍑哄懡浠ゅ垪琛ㄤ腑灞曠ず鍛戒护鏉冮檺
 */
#define     SHELL_HELP_SHOW_PERMISSION  1

/**
 * @brief 浣跨敤LF浣滀负鍛戒护琛屽洖杞﹁Е鍙�
 *        鍙互鍜孲HELL_ENTER_CR鍚屾椂寮�鍚�
 */
#define     SHELL_ENTER_LF              1

/**
 * @brief 浣跨敤CR浣滀负鍛戒护琛屽洖杞﹁Е鍙�
 *        鍙互鍜孲HELL_ENTER_LF鍚屾椂寮�鍚�
 */
#define     SHELL_ENTER_CR              1

/**
 * @brief 浣跨敤CRLF浣滀负鍛戒护琛屽洖杞﹁Е鍙�
 *        涓嶅彲浠ュ拰SHELL_ENTER_LF鎴朣HELL_ENTER_CR鍚屾椂寮�鍚�
 */
#define     SHELL_ENTER_CRLF            0

/**
 * @brief shell鍛戒护鍙傛暟鏈�澶ф暟閲�
 *        鍖呭惈鍛戒护鍚嶅湪鍐咃紝瓒呰繃8涓弬鏁板苟涓斾娇鐢ㄤ簡鍙傛暟鑷姩杞崲鐨勬儏鍐典笅锛岄渶瑕佷慨鏀规簮鐮�
 */
#define     SHELL_PARAMETER_MAX_NUMBER  32

/**
 * @brief 鍘嗗彶鍛戒护璁板綍鏁伴噺
 */
#define     SHELL_HISTORY_MAX_NUMBER    5

/**
 * @brief 鍙屽嚮闂撮殧(ms)
 *        浣胯兘瀹廯SHELL_LONG_HELP`鍚庢瀹忕敓鏁堬紝瀹氫箟鍙屽嚮tab琛ュ叏help鐨勬椂闂撮棿闅�
 */
#define     SHELL_DOUBLE_CLICK_TIME     200

/**
 * @brief 绠＄悊鐨勬渶澶hell鏁伴噺
 */
#define     SHELL_MAX_NUMBER            5

/**
 * @brief shell鏍煎紡鍖栬緭鍑虹殑缂撳啿澶у皬
 *        涓�0鏃朵笉浣跨敤shell鏍煎紡鍖栬緭鍑�
 */
#define     SHELL_PRINT_BUFFER          128

/**
 * @brief 鑾峰彇绯荤粺鏃堕棿(ms)
 *        瀹氫箟姝ゅ畯涓鸿幏鍙栫郴缁烼ick锛屽`HAL_GetTick()`
 * @note 姝ゅ畯涓嶅畾涔夋椂鏃犳硶浣跨敤鍙屽嚮tab琛ュ叏鍛戒护help锛屾棤娉曚娇鐢╯hell瓒呮椂閿佸畾
 */
#define     SHELL_GET_TICK()            0

/**
 * @brief shell榛樿鐢ㄦ埛
 */
#define     SHELL_DEFAULT_USER          "cvgshell"

/**
 * @brief shell榛樿鐢ㄦ埛瀵嗙爜
 *        鑻ラ粯璁ょ敤鎴蜂笉闇�瑕佸瘑鐮侊紝璁句负""
 */
#define     SHELL_DEFAULT_USER_PASSWORD ""

/**
 * @brief shell鑷姩閿佸畾瓒呮椂
 *        浣胯兘`SHELL_USING_AUTH`鐨勬儏鍐典笅鐢熸晥锛岃秴鏃跺悗浼氳嚜鍔ㄩ噸鏂伴攣瀹歴hell
 *        璁剧疆涓�0鏃跺叧闂嚜鍔ㄩ攣瀹氬姛鑳斤紝鏃堕棿鍗曚綅涓篳SHELL_GET_TICK()`鍗曚綅
 * @note 浣跨敤瓒呮椂閿佸畾蹇呴』淇濊瘉`SHELL_GET_TICK()`鏈夋晥
 */
#define     SHELL_LOCK_TIMEOUT          0 * 60 * 1000

#endif
