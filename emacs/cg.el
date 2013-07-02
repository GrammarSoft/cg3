; -*- coding:utf-8 -*-

; cg.el -- major mode for editing Constraint Grammar files

; See http://beta.visl.sdu.dk/constraint_grammar.html

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

;;; Send bug reports and feature requests to:
;;; Kevin Brubeck Unhammer <surname at fsfe dot org>

;;; Usage:
;; (autoload 'cg-mode "/path/to/cg.el"
;;  "cg-mode is a major mode for editing Constraint Grammar files."  t)
;; ; whatever file ending you use:
;; (add-to-list 'auto-mode-alist '("\\.cg3\\'" . cg-mode))

;;; I recommend using pabbrev-mode for tab-completion, and
;;; paredit-mode if you're used to it. However, if you have set names
;;; with hashes in them, like m#foo, paredit-mode may tell you that
;;; your parens are unbalenced if you have it in a mode hook. The
;;; reason is that such exceptions to the rule that # starts a comment
;;; are handled in font-lock, and fontification happens after
;;; run-mode-hooks. To get paredit-mode to turn on automatically in
;;; cg-mode, you could use this hack:

;; (add-hook 'cg-mode-hook
;; 	  (lambda nil
;; 	    (run-with-idle-timer 1 nil (lambda nil (paredit-mode 1)))))

;;; sh-mode has the same problem, so I don't feel up to fixing it...

;;; TODO:
;;; - different syntax highlighting for sets and tags (difficult)
;;; - use something like prolog-clause-start to define M-a/e etc.
;;; - run vislcg3 --show-unused-sets and buttonise with line numbers (like Occur does)
;;; - indentation function (based on prolog again?)
;;; - the rest of the keywords
;;; - keyword tab-completion
;;; - the quotes-within-quotes thing plays merry hell with
;;;   paredit-doublequote, write a new doublequote function?
;;; - font-lock-syntactic-keywords is obsolete since 24.1
;;; - derive cg-mode from prog-mode?
;;; - goto-set/list
;;; - show definition of set/list-at-point in modeline

(defconst cg-version "2013-07-02" "Version of cg-mode")

;;;============================================================================
;;;
;;; Define the formal stuff for a major mode named cg.
;;;

(defvar cg-mode-map (make-sparse-keymap)
  "Keymap for CG mode.")

(defgroup cg nil
  "Major mode for editing VISL CG-3 Constraint Grammar files."
  :tag "CG"
  :group 'languages)

;;;###autoload
(defcustom cg-command "vislcg3"
  "The vislcg3 command, e.g. \"/usr/local/bin/vislcg3\".

Buffer-local, so use `setq-default' if you want to change the
global default value. 

See also `cg-extra-args' and `cg-pre-pipe'."
  :group 'cg
  :type 'string)
(make-variable-buffer-local 'cg-extra-args)

;;;###autoload
(defcustom cg-extra-args "--trace"
  "Extra arguments sent to vislcg3 when running `cg-check'.

Buffer-local, so use `setq-default' if you want to change the
global default value. 

See also `cg-command'."
  :group 'cg
  :type 'string)
(make-variable-buffer-local 'cg-extra-args)

;;;###autoload
(defcustom cg-pre-pipe "cg-conv"
  "Pipeline to run before the vislcg3 command when testing a file
with `cg-check'. 

Buffer-local, so use `setq-default' if you want to change the
global default value. If you want to set it on a per-file basis,
put a line like

# -*- cg-pre-pipe: \"lt-proc foo.bin | cg-conv\"; othervar: value; -*-

in your .cg3/.rlx file.

See also `cg-command' and `cg-post-pipe'."
  :group 'cg
  :type 'string)
(make-variable-buffer-local 'cg-pre-pipe)

;;;###autoload
(defcustom cg-post-pipe ""
  "Pipeline to run after the vislcg3 command when testing a file
with `cg-check'. 

Buffer-local, so use `setq-default' if you want to change the
global default value. If you want to set it on a per-file basis,
put a line like

# -*- cg-post-pipe: \"cg-conv --out-apertium | lt-proc -b foo.bin\"; -*-

in your .cg3/.rlx file.

See also `cg-command' and `cg-pre-pipe'."
  :group 'cg
  :type 'string)
(make-variable-buffer-local 'cg-post-pipe)


;;;###autoload
(defcustom cg-indentation 8
  "The width for indentation in Constraint Grammar mode."
  :type 'integer
  :group 'cg)
(put 'cg-indentation 'safe-local-variable 'integerp)

(defvar cg-font-lock-keywords-1
  (let ((<word>? "\\(?:\"<[^>]+>\"\\)?"))
    `(("^[ \t]*\\(LIST\\|SET\\|TEMPLATE\\)[ \t]+\\(\\(\\sw\\|\\s_\\)+\\)"
       (1 font-lock-keyword-face)
       (2 font-lock-variable-name-face))
      ("^[ \t]*\\(MAPPING-PREFIX\\|DELIMITERS\\|SOFT-DELIMITERS\\)"
       1 font-lock-keyword-face)
      ("^[ \t]*\\(SECTION\\|AFTER-SECTIONS\\|BEFORE-SECTIONS\\|MAPPINGS\\|CONSTRAINTS\\|CORRECTIONS\\)"
       1 font-lock-warning-face)
      (,(concat "^[ \t]*" <word>? "[ \t]*\\(SETPARENT\\|SETCHILD\\|ADDRELATIONS?\\|SETRELATIONS?\\|REMRELATIONS?\\|SUBSTITUTE\\|ADDCOHORT\\|REMCOHORT\\|COPY\\|MAP\\|IFF\\|ADD\\|SELECT\\|REMOVE\\)\\(\\(:\\(\\s_\\|\\sw\\)+\\)?\\)")
       (1 font-lock-keyword-face)
       (2 font-lock-variable-name-face))
      ("[ \t\n]\\([+-]\\)[ \t\n]"
       1 font-lock-function-name-face)))
  "Subdued level highlighting for CG mode.")

(defconst cg-font-lock-keywords-2
  (append cg-font-lock-keywords-1
	  '(("\\<\\(&&\\(\\s_\\|\\sw\\)+\\)\\>"
	     (1 font-lock-variable-name-face))
	    ("\\<\\(\\$\\$\\(\\s_\\|\\sw\\)+\\)\\>"
	     (1 font-lock-variable-name-face))
	    ("\\<\\(NOT\\|NEGATE\\|NONE\\|LINK\\|BARRIER\\|CBARRIER\\|OR\\|TARGET\\|IF\\|AFTER\\|TO\\|[psc][lroOxX]*\\)\\>"
	     1 font-lock-function-name-face)
	    ("\\B\\(\\^\\)"		; fail-fast
	     1 font-lock-function-name-face)))
  "Gaudy level highlighting for CG modes.")

(defvar cg-font-lock-keywords cg-font-lock-keywords-1
  "Default expressions to highlight in CG modes.")

(defvar cg-mode-syntax-table
  (let ((table (make-syntax-table)))
    (modify-syntax-entry ?# "<" table)
    (modify-syntax-entry ?\n ">#" table)
    ;; todo: better/possible to conflate \\s_ and \\sw into one class?
    (modify-syntax-entry ?@ "_" table)
    ;; using syntactic keywords for "
    (modify-syntax-entry ?\" "." table)
    (modify-syntax-entry ?» "." table)
    (modify-syntax-entry ?« "." table)
    table))

;;;###autoload
(defun cg-mode ()
  "Major mode for editing Constraint Grammar files.
Only does basic syntax highlighting at the moment."
  (interactive)
  (kill-all-local-variables)
  (setq major-mode 'cg-mode
	mode-name "CG")
  (use-local-map cg-mode-map)
  (make-local-variable 'comment-start)
  (make-local-variable 'comment-start-skip)
  (make-local-variable 'font-lock-defaults)
  (make-local-variable 'indent-line-function)
  (setq comment-start "#"
	comment-start-skip "#+[\t ]*"
	font-lock-defaults
	`((cg-font-lock-keywords cg-font-lock-keywords-1 cg-font-lock-keywords-2)
	  nil				; KEYWORDS-ONLY
	  'case-fold ; some keywords (e.g. x vs X) are case-sensitive,
		     ; but that doesn't matter for highlighting
	  ((?/ . "w") (?~ . "w") (?. . "w") (?- . "w") (?_ . "w"))
	  nil ;	  beginning-of-line		; SYNTAX-BEGIN
	  (font-lock-syntactic-keywords . cg-font-lock-syntactic-keywords)
	  (font-lock-syntactic-face-function . cg-font-lock-syntactic-face-function)))
  (make-local-variable 'cg-mode-syntax-table)
  (set-syntax-table cg-mode-syntax-table)
  (set (make-local-variable 'parse-sexp-ignore-comments) t)
  (set (make-local-variable 'parse-sexp-lookup-properties) t)
  (setq indent-line-function #'cg-indent-line)
  (easy-mmode-pretty-mode-name 'cg-mode " cg")
  (when font-lock-mode
    (setq font-lock-set-defaults nil)
    (font-lock-set-defaults)
    (font-lock-fontify-buffer))
  (add-hook 'after-change-functions #'cg-after-change nil 'buffer-local)
  (run-mode-hooks #'cg-mode-hook))


(defconst cg-font-lock-syntactic-keywords
  ;; We can have ("words"with"quotes"inside"")! Quote rule: is it a ",
  ;; if yes then jump to next unescaped ". Then regardless, jump to
  ;; next whitespace, but don't cross an unescaped )
  '(("\\(\"\\)[^\"\n]*\\(?:\"\\(?:\\\\)\\|[^) \n\t]\\)*\\)?\\(\"\\)\\(r\\(i\\)?\\)?[); \n\t]"
     (1 "\"")
     (2 "\""))
    ;; A `#' begins a comment when it is unquoted and at the beginning
    ;; of a word; otherwise it is a symbol.
    ;; For this to work, we also add # into the syntax-table as a
    ;; comment, with \n to turn it off, and also need
    ;; (set (make-local-variable 'parse-sexp-lookup-properties) t)
    ;; to avoid parser problems.
    ("[^|&;<>()`\\\"' \t\n]\\(#+\\)" 1 "_")
    ;; fail-fast, at the beginning of a word:
    ("[( \t\n]\\(\\^\\)" 1 "'")))

(defun cg-font-lock-syntactic-face-function (state)
  "Determine which face to use when fontifying syntactically. See
`font-lock-syntactic-face-function'.

TODO: something like
	((= 0 (nth 0 state)) font-lock-variable-name-face)
would be great to differentiate SETs from their members, but it
seems this function only runs on comments and strings..."
  (cond ((nth 3 state)
	 (if
	     (save-excursion
	       (goto-char (nth 8 state))
	       (re-search-forward "\"[^\"\n]*\\(\"\\(\\\\)\\|[^) \n\t]\\)*\\)?\"\\(r\\(i\\)?\\)?[); \n\t]")
	       (and (match-string 1)
		    (not (equal ?\\ (char-before (match-beginning 1))))
		    ;; TODO: make next-error hit these too
		    ))
	     'cg-string-warning-face
	   font-lock-string-face))
 	(t font-lock-comment-face)))

(defface cg-string-warning-face
  '((((class grayscale) (background light)) :foreground "DimGray" :slant italic :underline "orange")
    (((class grayscale) (background dark))  :foreground "LightGray" :slant italic :underline "orange")
    (((class color) (min-colors 88) (background light)) :foreground "VioletRed4" :underline "orange")
    (((class color) (min-colors 88) (background dark))  :foreground "LightSalmon" :underline "orange")
    (((class color) (min-colors 16) (background light)) :foreground "RosyBrown" :underline "orange")
    (((class color) (min-colors 16) (background dark))  :foreground "LightSalmon" :underline "orange")
    (((class color) (min-colors 8)) :foreground "green" :underline "orange")
    (t :slant italic))
  "CG mode face used to highlight troublesome strings with unescaped quotes in them."
  :group 'cg-mode)




;;; Indentation

(defvar cg-kw-list
  '("SUBSTITUTE" "IFF"
    "ADDCOHORT" "REMCOHORT"
    "COPY"
    "MAP"    "ADD"
    "SELECT" "REMOVE"
    "LIST"   "SET"
    "SETPARENT"    "SETCHILD"
    "ADDRELATION"  "REMRELATION"
    "ADDRELATIONS" "REMRELATIONS"
    ";"))

(defun cg-calculate-indent ()
  "Return the indentation for the current line."
;;; idea from sh-mode, use font face?
;; (or (and (boundp 'font-lock-string-face) (not (bobp))
;; 		 (eq (get-text-property (1- (point)) 'face)
;; 		     font-lock-string-face))
;; 	    (eq (get-text-property (point) 'face) sh-heredoc-face))
  (let ((origin (point))
	(old-case-fold-search case-fold-search))
    (setq case-fold-search nil)		; for re-search-backward
    (save-excursion
      (let ((kw-pos (progn
		      (goto-char (1- (or (search-forward ";" (line-end-position) t)
					 (line-end-position))))
		      (re-search-backward (regexp-opt cg-kw-list) nil 'noerror))))
	(setq case-fold-search old-case-fold-search)
	(when kw-pos
	  (let* ((kw (match-string-no-properties 0)))
	    (if (and (not (equal kw ";"))
		     (> origin (line-end-position)))
		cg-indentation
	      0)))))))

(defun cg-indent-line ()
  "Indent the current line. Very simple indentation: lines with
keywords from `cg-kw-list' get zero indentation, others get one
indentation."
  (interactive)
  (let ((indent (cg-calculate-indent))
	(pos (- (point-max) (point))))
    (when indent
      (beginning-of-line)
      (skip-chars-forward " \t")
      (indent-line-to indent)
      ;; If initial point was within line's indentation,
      ;; position after the indentation.  Else stay at same point in text.
      (if (> (- (point-max) pos) (point))
	  (goto-char (- (point-max) pos))))))


;;; Interactive functions:

(defvar cg-occur-history nil)
(defvar cg-occur-prefix-history nil)
(defvar cg-goto-history nil)

(defun cg-permute (input)
  "From http://www.emacswiki.org/emacs/StringPermutations"
  (require 'cl)
  (if (null input)
      (list input)
    (mapcan (lambda (elt)
	      (mapcan (lambda (p)
			(list (cons elt p)))
		      (cg-permute (remove* elt input :count 1))))
	    input)))

(defun cg-read-arg (prompt history)
  (let* ((default (car history))
	 (input
	  (read-from-minibuffer
	   (concat prompt
		   (if default
		       (format " (default: %s): " (query-replace-descr default))
		     ": "))
	   nil
	   nil
	   nil
	   (quote history)
	   default)))
    (if (equal input "")
	default
      input)))

(defun cg-occur-list (&optional prefix words)
  "Do an occur-check for the left-side of a LIST/SET
assignment. `words' is a space-separated list of words which
*all* must occur between LIST/SET and =. Optional prefix argument
`prefix' lets you specify a prefix to the name of LIST/SET.

This is useful if you have a whole bunch of this stuff:
LIST subst-mask/fem = (n m) (np m) (n f) (np f) ;
LIST subst-mask/fem-eint = (n m sg) (np m sg) (n f sg) (np f sg) ;
etc."
  (interactive (list (when current-prefix-arg
		       (cg-read-arg
			"Word to occur between LIST/SET and disjunction"
			cg-occur-prefix-history))
		     (cg-read-arg
		       "Words to occur between LIST/SET and ="
		       cg-occur-history)))
  (let* ((words-perm (cg-permute (split-string words " " 'omitnulls)))
	 ;; can't use regex-opt because we need .* between the words
	 (perm-disj (mapconcat (lambda (word)
				 (mapconcat 'identity word ".*"))
			       words-perm
			       "\\|")))
    (setq cg-occur-history (cons words cg-occur-history))
    (setq cg-occur-prefix-history (cons prefix cg-occur-prefix-history))
    (let ((tmp regexp-history))
      (occur (concat "\\(LIST\\|SET\\) +" prefix ".*\\(" perm-disj "\\).*="))
      (setq regexp-history tmp))))

(defun cg-goto-rule (&optional input)
  "Go to the line number of the rule described by `input', where
`input' is the rule info from vislcg3 --trace.  E.g. if `input'
is \"SELECT:1022:rulename\", go to the rule on line number
1022. Interactively, use a prefix argument to paste `input'
manually, otherwise this function uses the most recently copied
line in the X clipboard.

This makes switching between the terminal and the file slightly
faster (since double-clicking the rule info -- in Konsole at
least -- selects the whole string \"SELECT:1022:rulename\").

Ideally we should have some sort of comint interpreter to make
trace output clickable, but since we're often switching between
_several_ CG files in a pipeline, that could get complicated
before getting useful..."
  (interactive (list (when current-prefix-arg
		       (cg-read-arg "Paste rule info from --trace here: "
				    cg-goto-history))))
  (let ((errmsg (if input (concat "Unrecognised rule/trace format: " input)
		  "X clipboard does not seem to contain vislcg3 --trace rule info"))
	(rule (or input (with-temp-buffer
			  (yank)
			  (buffer-substring-no-properties (point-min)(point-max))))))
    (if (string-match
	 "\\(\\(select\\|iff\\|remove\\|map\\|addcohort\\|remcohort\\|copy\\|add\\|substitute\\):\\)?\\([0-9]+\\)"
	 rule)
	(progn (goto-line (string-to-number (match-string 3 rule)))
	       (setq cg-goto-history (cons rule cg-goto-history)))
      (message errmsg))))

;;; "Flycheck" ----------------------------------------------------------------
(require 'compile)

(defvar cg-file nil
  "Private, used in `cg-output-mode' buffers to record which
  user-edited grammar file was sent to the compilation")
(defvar cg-tmp nil     ; TODO: could use cg-file iff buffer-modified-p
  "Private, used in `cg-output-mode' buffers to record which
  temporary file was sent in lieu of `cg-file' to the
  compilation (in case the buffer of `cg-file' was not saved)")

(unless (fboundp 'file-name-base)	; shim for 24.3 function
  (defun file-name-base (&optional filename)
    (let ((filename (or filename (buffer-file-name))))
      (file-name-nondirectory (file-name-sans-extension filename)))))

(defun cg-edit-input ()
  "Open a buffer to edit the input sent when running `cg-check'."
  ;; TODO: save window configuration here, restore on C-c C-c
  (interactive)
  (pop-to-buffer (cg-input-buffer (buffer-file-name))))

(defvar cg-input-mode-map (make-sparse-keymap)
  "Keymap for CG input mode.")

(define-derived-mode cg-input-mode fundamental-mode "CG-in"
  "Input for `cg-mode' buffers."
  (use-local-map cg-input-mode-map))

(defun cg-input-buffer (file)
  (let ((buf (get-buffer-create (concat "*CG input for " (file-name-base file) "*"))))
    (with-current-buffer buf
      (cg-input-mode)
      (setq cg-file file))
    buf))

(defun cg-get-file ()
  (list cg-file))

(defconst cg-output-regexp-alist
  `(("\\(?:SETPARENT\\|SETCHILD\\|ADDRELATIONS?\\|SETRELATIONS?\\|REMRELATIONS?\\|SUBSTITUTE\\|ADDCOHORT\\|ADDCOHORT-AFTER\\|ADDCOHORT-BEFORE\\|REMCOHORT\\|COPY\\|MAP\\|IFF\\|ADD\\|SELECT\\|REMOVE\\):\\([^ \n\t:]+\\)\\(?::[^ \n\t]+\\)?"
     ,#'cg-get-file 1 nil 1)
    ("^Warning: .*?line \\([0-9]+\\)"
     ,#'cg-get-file 1 nil 1)
    ("^Warning: .*"
     ,#'cg-get-file nil nil 1)
    ("^Error: .*?line \\([0-9]+\\)"
     ,#'cg-get-file 1 nil 2)
    ("^Error: .*"
     ,#'cg-get-file nil nil 2)
    (".*?line \\([0-9]+\\)"		; some error messages span several lines
     ,#'cg-get-file 1 nil 2))
  "Regexp used to match vislcg3 --trace hits. See
`compilation-error-regexp-alist'.")
;; TODO: highlight strings and @'s and #1->0's in cg-output-mode ?

;;;###autoload
(defcustom cg-output-setup-hook nil
  "List of hook functions run by `cg-output-process-setup' (see
`run-hooks')."
  :type 'hook
  :group 'cg-mode)

(defun cg-output-process-setup ()
  "Runs `cg-output-setup-hook' for `cg-check'. That hook is
useful for doing things like
 (setenv \"PATH\" (concat \"~/local/stuff\" (getenv \"PATH\")))"
  (run-hooks #'cg-output-setup-hook))

(define-compilation-mode cg-output-mode "CG-out"
  "Major mode for output of Constraint Grammar compilations and
runs."
  (set (make-local-variable 'compilation-skip-threshold)
       1)
  (set (make-local-variable 'compilation-error-regexp-alist)
       cg-output-regexp-alist)
  (set (make-local-variable 'cg-file)
       nil)
  (set (make-local-variable 'cg-tmp)
       nil)
  (set (make-local-variable 'compilation-disable-input)
       nil)
  ;; compilation-directory-matcher can't be nil, so we set it to a regexp that
  ;; can never match.
  (set (make-local-variable 'compilation-directory-matcher)
       '("\\`a\\`"))
  (set (make-local-variable 'compilation-process-setup-function)
       #'cg-output-process-setup)
  ;; (add-hook 'compilation-filter-hook 'cg-output-filter nil t)
  ;; We send text to stdin:
  (set (make-local-variable 'compilation-disable-input)
       nil)
  (set (make-local-variable 'compilation-finish-functions)
       (list #'cg-check-finish-function)))

(defun cg-after-change (a b c)
  ;; TODO: create run cg-check if it's over 2 seconds since last? Or
  ;; is that just annoying? Perhaps as a save-hook? Or perhaps just
  ;; compile on save?
  ; (cg-check)
  )


(defun cg-output-buffer-name (mode)
  (if (equal mode "cg-output")
      (concat "*CG output for " (file-name-base cg-file) "*")
    (error "Unexpected mode %S" mode)))

(defun cg-check ()
  "Run vislcg3 --trace on the buffer (a temporary file is created
in case you haven't saved yet).

If you've set `cg-pre-pipe', input will first be sent through
that. Set your test input sentence(s) with `cg-edit-input'. If
you want to send a whole file instead, just set `cg-pre-pipe' to
something like
\"zcat corpus.gz | lt-proc analyser.bin | cg-conv\".

Similarly, `cg-post-pipe' is run on output."
  (interactive)
  (let* ((file (buffer-file-name))
	 (tmp (make-temp-file "cg."))
	 (pre-pipe (if (and cg-pre-pipe (not (equal "" cg-pre-pipe)))
		       (concat cg-pre-pipe " | ")
		     ""))
	 (post-pipe (if (and cg-post-pipe (not (equal "" cg-post-pipe)))
			(concat " | " cg-post-pipe)
		      ""))
	 (cmd (concat
	       pre-pipe			; TODO: cache pre-output!
	       cg-command " " cg-extra-args " --grammar " tmp
	       post-pipe))
	 (in (cg-input-buffer file))
	 (out (progn (write-region (point-min) (point-max) tmp)
		     (compilation-start
		      cmd
		      'cg-output-mode
		      'cg-output-buffer-name)))
	 (proc (get-buffer-process out)))
    (with-current-buffer out
      (setq cg-tmp tmp)
      (setq cg-file file))
    (with-current-buffer in
      (process-send-region proc (point-min) (point-max))
      (process-send-string proc "\n")
      (process-send-eof proc))
    (display-buffer out)))

(defun cg-check-finish-function (buffer change)
  ;; Note: this makes `recompile' not work, which is why `g' is
  ;; rebound in `cg-output-mode'
  (with-current-buffer buffer
    (delete-file cg-tmp)))

(defun cg-back-to-file-and-edit-input ()
  (interactive)
  (cg-back-to-file)
  (cg-edit-input))

(defun cg-back-to-file ()
  (interactive)
  (bury-buffer)
  (pop-to-buffer (find-buffer-visiting cg-file)))

(defun cg-back-to-file-and-check ()
  (interactive)
  (cg-back-to-file)
  (cg-check))




;;; Keybindings ---------------------------------------------------------------
(define-key cg-mode-map (kbd "C-c C-o") #'cg-occur-list)
(define-key cg-mode-map (kbd "C-c g") #'cg-goto-rule)
(define-key cg-mode-map (kbd "C-c C-c") #'cg-check)
(define-key cg-mode-map (kbd "C-c C-i") #'cg-edit-input)
(define-key cg-output-mode-map (kbd "C-c C-i") #'cg-back-to-file-and-edit-input)
(define-key cg-output-mode-map (kbd "i") #'cg-back-to-file-and-edit-input)
(define-key cg-output-mode-map (kbd "g") #'cg-back-to-file-and-check)

(define-key cg-input-mode-map (kbd "C-c C-c") #'cg-back-to-file-and-check)
(define-key cg-output-mode-map (kbd "C-c C-c") #'cg-back-to-file)

(define-key cg-output-mode-map "n" 'next-error-no-select)
(define-key cg-output-mode-map "p" 'previous-error-no-select)

;;; Run hooks -----------------------------------------------------------------
(run-hooks #'cg-load-hook)

(provide 'cg)

;;;============================================================================

;;; cg.el ends here
