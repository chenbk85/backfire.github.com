;;最后更新: 2010年12月22日 18:24:09 by smyang

(when window-system
  (set-frame-font "Monaco-10")
  (set-fontset-font (frame-parameter nil 'font)
                    'han '("WenQuanYi Micro Hei" . "unicode-bmp")))
  ;; (set-fontset-font (frame-parameter nil 'font)
  ;;                   'symbol '("WenQuanYi Micro Hei" . "ISO10646-1"))
  ;; (set-fontset-font (frame-parameter nil 'font)
  ;;                   'cjk-misc '("WenQuanYi Micro Hei" . "ISO10646-1"))
  ;; (set-fontset-font (frame-parameter nil 'font)
  ;;                   'bopomofo '("WenQuanYi Micro Hei" . "ISO10646-1")))
;;字体设置

(require 'tmtheme)
(setq tmtheme-directory "~/.emacs.d/site-lisp/tmthemes")
(tmtheme-scan)
(tmtheme-Monokai)
;;主题设置

(if (facep 'mode-line)
    (set-face-attribute 'mode-line nil :foreground "Black" :background "Wheat"))
;;设置状态栏的颜色

(set-scroll-bar-mode nil)
;;取消滚动栏

(tool-bar-mode -1)
;;取消工具栏

(menu-bar-mode -1)
;;取消菜单栏

(column-number-mode t)
;;显示列号

(setq initial-scratch-message nil)
;;取消初始的scratch

(setq inhibit-startup-message t)
;;关闭emacs启动时的画面

(setq gnus-inhibit-startup-message t)
;;关闭gnus启动时的画面

(setq frame-title-format "%b")
;;在标题栏显示buffer的名字

(ido-mode t)

(setq ido-enable-tramp-completion t)
;;ido模式中，使用tramp的补全方式

(fset 'yes-or-no-p 'y-or-n-p)
;;改变 Emacs 固执的要你回答 yes 的行为。按 y 或空格键表示 yes，n 表示 no

(setq kill-buffer-query-functions
      (remove 'process-kill-buffer-query-function kill-buffer-query-functions))
;;kill有process运行的buffer时不用确认

(setq-default kill-whole-line t)
;;在行首 C-k 时，同时删除该行。

(require 'uniquify)
(setq uniquify-buffer-name-style 'forward)
;;当有两个文件名相同的缓冲时，使用前缀的目录名做 buffer 名字，不用原来的foobar<?> 形式

(global-font-lock-mode 1)
;;开启语法高亮

(blink-cursor-mode -1)
;;光标不要闪烁

(show-paren-mode 1)
;;高亮显示匹配的括号

(icomplete-mode 1)
;;给出用 M-x foo-bar-COMMAND 输入命令的提示

(setq scroll-margin 3 scroll-conservatively 10000)
;;防止页面滚动时跳动，scroll-margin 3 可以在靠近屏幕边沿3行时就开始滚动，可以很好的看到上下文

(setq enable-recursive-minibuffers t)
;;可以递归的使用 minibuffer


(display-time-mode 1)
;;启用时间显示设置，在minibuffer上面的那个杠上

(setq display-time-24hr-format t)
;;时间使用24小时制

(setq display-time-day-and-date t)
;;时间显示包括日期和具体时间

(setq display-time-interval 10)
;;时间的变化频率
