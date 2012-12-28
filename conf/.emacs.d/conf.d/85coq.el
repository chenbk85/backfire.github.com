;; 最后更新: 2010年1月13日 21:17:14 by smyang

(load-file "~/.emacs.d/site-lisp/ProofGeneral/generic/proof-site.elc")


(setq ltac-flag 1)

(defun coq-Toggle-Ltac-Debug ()
  (interactive)
  (if (eq ltac-flag 1)
      (progn
        (setq ltac-flag 0)
        (proof-shell-invisible-command "Set Ltac Debug.")
        (message "Set Ltac Debug"))
    (progn
      (setq ltac-flag 1)
      (proof-shell-invisible-command "Unset Ltac Debug.")
      (message "Unset Ltac Debug"))))

  (global-set-key (kbd "C-c C-a C-d") 'coq-Toggle-Ltac-Debug)
  (setq coq-default-undo-limit 999)
  (setq proof-three-window-enable t)
  (setq proof-script-fly-past-comments t)
  (setq proof-splash-enable nil)



;;;; coq-indent.el -- hacked by guoyu

;; This function is very complex, indentation of a line (inside an
;; expression) is determined by the beginning of this line (current
;; point) and the indentation items found at previous pertinent (not
;; comment, not string, not empty) line. Sometimes we even need the
;; previous line of previous line.

;; prevcol is the indentation column of the previous line, prevpoint
;; is the indentation point of previous line, record tells if we are
;; inside the accolades of a record.


(defun coq-pg-hack ()
(defun coq-indent-expr-offset (kind prevcol prevpoint record)
  "Returns the indentation column of the current line.
This function indents a *expression* line (a line inside of a command).  Use
`coq-indent-command-offset' to indent a line belonging to a command.  The fourth
argument must be t if inside the {}s of a record, nil otherwise."

  (let ((pt (point)) real-start)
    (save-excursion
      (setq real-start (coq-find-real-start)))
  
    (cond

     ;; at a ) -> same indent as the *line* of corresponding (
     ((proof-looking-at-safe coq-indent-closepar-regexp)
      (save-excursion (coq-find-unclosed 1 real-start)
                      (back-to-indentation)
                      (current-column)))

     ;; at an end -> same indent as the corresponding match or Case
     ((proof-looking-at-safe coq-indent-closematch-regexp)
      (save-excursion (coq-find-unclosed 1 real-start)
                      (current-column)))

     ;; if we find a "|" we indent from the first unclosed
     ;; or from the command start (if we are in an Inductive type)
     ((proof-looking-at-safe coq-indent-pattern-regexp)
      (save-excursion
        (coq-find-unclosed 1 real-start)
        (cond
         ((proof-looking-at-safe "\\s(")
          (+ (current-indentation) proof-indent))
         ((proof-looking-at-safe (proof-ids-to-regexp coq-keywords-defn))
          (progn (message "hack") (+ (current-column) proof-indent)))
         (t (+ (current-column) proof-indent)))))

     ;; if we find a "then" we indent from the first unclosed if
     ;; or from the command start (should not happen)
     ((proof-looking-at-safe "\\<then\\>")
      (save-excursion
        (coq-find-unclosed 1 real-start "\\<if\\>" "\\<then\\>")
        (back-to-indentation)
        (+ (current-column) proof-indent)))

     ;; if we find a "else" we indent from the first unclosed if
     ;; or from the command start (should not happen)
     ((proof-looking-at-safe "\\<else\\>")
      (save-excursion
        (coq-find-unclosed 1 real-start "\\<then\\>" "\\<else\\>")
        (coq-find-unclosed 1 real-start "\\<if\\>" "\\<then\\>")
        (back-to-indentation)
        (+ (current-column) proof-indent)))

     ;; there is an unclosed open in the previous line
     ;; -> same column if match
     ;; -> same indent than prev line + indent if (
     ((coq-find-unclosed 1 prevpoint
                         coq-indent-openmatch-regexp
                         coq-indent-closematch-regexp)
      (let ((base (if (proof-looking-at-safe coq-indent-openmatch-regexp)
                      (current-column)
                    prevcol)))
        (+ base proof-indent)))


;; there is an unclosed '(' in the previous line -> prev line indent + indent
;;	  ((and (goto-char pt) nil)) ; just for side effect: jump to initial point
;;	  ((coq-find-unclosed 1 prevpoint
;;            coq-indent-openpar-regexp
;;            coq-indent-closepar-regexp)
;;		(+ prevcol proof-indent))

     ((and (goto-char prevpoint) nil)) ; just for side effect: jump to previous line
	
     ;; find the last unopened ) -> indentation of line + indent
     ((coq-find-last-unopened 1 pt) ; side effect, nil if no unopenned found
      (save-excursion
        (coq-find-unclosed 1 real-start)
        (back-to-indentation)
        (current-column)))

     ;; just for side effect: jumps to end of previous line
     ((and (goto-char prevpoint)
           (or (and (end-of-line) nil)
               (and (forward-char -1) t)) nil))
     
     ((and  (proof-looking-at-safe ";") ;prev line has ";" at the end
            record)                     ; and we are inside {}s of a record
      (save-excursion 
        (coq-find-unclosed 1 real-start)
        (back-to-indentation)
        (+ (current-column) proof-indent)))

     ;; just for side effect: jumps to end of previous line
     ((and (goto-char prevpoint)  (not (= (coq-back-to-indentation-prevline) 0))
           (or (and (end-of-line) nil)
               (and (forward-char -1) t)) nil))
     
     ((and (proof-looking-at-safe ";") ;prev prev line has ";" at the end
           record)                     ; and we are inside {}s of a record
      (save-excursion (+ prevcol proof-indent)))

     ((and (goto-char pt) nil)) ;; just for side effect: go back to initial point

     ;; There is a indent keyword (fun, forall etc)
     ;; and we are not in {}s of a record just after a ";"
     ((coq-find-at-same-level-zero prevpoint coq-indent-kw) 
      (+ prevcol proof-indent))

     ((and (goto-char prevpoint) nil)) ;; just for side effect: go back to previous line
     ;; "|" at previous line
     ((proof-looking-at-safe coq-indent-pattern-regexp)
      (+ prevcol proof-indent))
     
     (t prevcol))))
(message "coq-pg-hack"))

  (add-hook 'coq-mode-hook 'coq-pg-hack)

  (defun new-coq-show-first-goal ()
    (save-excursion 
      (save-selected-window 
        (let ((window (get-buffer-window
                       proof-goals-buffer)))
          (if (window-live-p window)
              (progn
                (select-window window)
                (search-forward-regexp "subgoal 2\\|\\'"); find snd goal or buffer end
                (beginning-of-line)
                ))))
;;       (message "hack")
      ))
;;   (coq-show-first-goal)
;;   (remove-hook 'proof-shell-handle-delayed-output-hook 'new-coq-show-first-goal)
  (add-hook 'proof-shell-handle-delayed-output-hook 'new-coq-show-first-goal t)
