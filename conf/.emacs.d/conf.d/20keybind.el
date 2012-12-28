;; 最后更新: 2010年1月13日 21:29:35 by smyang

(global-set-key [f2] 'dired-jump)

(global-set-key [f5] 'byte-compile-file)

(global-set-key [f10] 'undo)
;;F10为撤销

(global-set-key [f11] 'revert-buffer)
;;F11为revert-buffer

(global-set-key [f12] 'calendar)
;; Emacs 的日历系统

(global-set-key (kbd "M-g") 'goto-line)
;;设置M-g为goto-line

(global-set-key (kbd "C-SPC") 'nil)
;;取消control+space键的绑定，系统设定ctrl+space为切换输入法

(defun my-swap-buffers ()
  "Swap the buffers in the window and the next other window."
  (interactive)
  (let ((my-orig-win-buffer (buffer-name)))
    (other-window 1)
    (let ((my-other-win-buffer (buffer-name)))
      (switch-to-buffer my-orig-win-buffer)
      (other-window 1)
      (switch-to-buffer my-other-win-buffer)))
  (other-window 1))
(global-set-key "\C-cs" 'my-swap-buffers)
;;; swap buffer |a|b|===>|b|a|

(global-set-key "\C-x\C-b" 'electric-buffer-list)
;; C-x C-b 缺省的绑定很不好用，改成一个比较方便的 electric-buffer-list，执行
;; 之后：
;;     光标自动转到 Buffer List buffer 中；
;;     n, p   上下移动选择 buffer；
;;     S      保存改动的 buffer；
;;     D      删除 buffer。
;; 除此之外，不错的选择还有 ibuffer，不妨试试 M-x ibuffer

;;windmove 绑定
(global-set-key (kbd "C-x <left>")  'windmove-left)
(global-set-key (kbd "C-x <up>")    'windmove-up)
(global-set-key (kbd "C-x <right>") 'windmove-right)
(global-set-key (kbd "C-x <down>")  'windmove-down)

