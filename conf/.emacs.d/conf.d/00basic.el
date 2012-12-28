;; 最后更新: 2010年1月13日 20:14:05 by smyang


(setq user-full-name "S.M.Yang")
(setq user-mail-address "smyang.ustc@gmail.com")
;;个人信息


(setq kill-ring-max 1000)
;;设置粘贴缓冲条目数量.用一个很大的kill ring(最多的记录个数). 这样防止我不小心删掉重要的东西

(setq make-backup-files nil)
;;设定不产生备份文件

(auto-compression-mode 1)
;;打开压缩文件时自动解压缩

(setq x-select-enable-clipboard t)
;;允许emacs和外部其他程序的粘贴

(setq major-mode 'text-mode)
;;缺省模式为 text-mode

(global-auto-revert-mode 1)
;;自动更新buffer

(setq ido-file-extensions-order (quote (".sml" ".tex" ".v")))
;;设置.sml/.tex/.v文件优先打开

(setq completion-ignored-extensions (quote (".d" ".vo" ".grm.sml" ".lex.sml" ".glob" "CM/" ".o" "~" ".bin" ".bak" ".obj" ".map" ".ico" ".pif" ".lnk" ".a" ".ln" ".blg" ".bbl" ".dll" ".drv" ".vxd" ".386" ".elc" ".lof" ".glo" ".idx" ".lot" ".svn/" ".hg/" ".git/" ".bzr/" "CVS/" "_darcs/" "_MTN/" ".fmt" ".tfm" ".class" ".fas" ".lib" ".mem" ".x86f" ".sparcf" ".fasl" ".ufsl" ".fsl" ".dxl" ".pfsl" ".dfsl" ".p64fsl" ".d64fsl" ".dx64fsl" ".lo" ".la" ".gmo" ".mo" ".toc" ".aux" ".cp" ".fn" ".ky" ".pg" ".tp" ".vr" ".cps" ".fns" ".kys" ".pgs" ".tps" ".vrs" ".pyc" ".pyo")))
;;忽略这些后缀名的文件

(setq bookmark-save-flag 1)
;; 每当设置书签的时候都保存书签文件，否则只在你退出 Emacs 时保存。

(setq bookmark-default-file "~/.emacs.d/bookmark")
;; 缺省书签文件的路径及文件名。

(add-hook 'shell-mode-hook 'ansi-color-for-comint-mode-on)
;; shell 中打开 ansi-color 支持。

(add-hook 'write-file-hooks 'time-stamp)
(setq time-stamp-start "最后更新:[     ]+\\\\?")
(setq time-stamp-end "\n")
(setq time-stamp-format "%:y年%:m月%:d日 %02H:%02M:%02S by %:u")
;; 保存文件时记录修改时间