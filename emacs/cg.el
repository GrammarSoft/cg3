;;; cg.el --- major mode for editing Constraint Grammar files

;; Copyright (C) 2010-2013 Kevin Brubeck Unhammer

;; Author: Kevin Brubeck Unhammer <unhammer@fsfe.org>
;; Version: 0.1.6
;; Url: http://beta.visl.sdu.dk/constraint_grammar.html
;; Keywords: languages

;; This file is not part of GNU Emacs.

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
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; Usage:
;;
;; (autoload 'cg-mode "/path/to/cg.el"
;;  "cg-mode is a major mode for editing Constraint Grammar files."  t)
;; (add-to-list 'auto-mode-alist '("\\.cg3\\'" . cg-mode))
;; ; Or if you use a non-standard file suffix, e.g. .rlx:
;; (add-to-list 'auto-mode-alist '("\\.rlx\\'" . cg-mode))

;; I recommend using autocomplete-mode for tab-completion, and
;; smartparens-mode if you're used to it (paredit-mode does not work
;; well if you have set names with the # character in them). Both are
;; available from MELPA (see http://melpa.milkbox.net/).

;; TODO:
;; - optionally highlight any LIST/SET without ; at the end
;; - different syntax highlighting for sets and tags (difficult)
;; - use something like prolog-clause-start to define M-a/e etc.
;; - run vislcg3 --show-unused-sets and buttonise with line numbers (like Occur does)
;; - indentation function (based on prolog again?)
;; - the rest of the keywords
;; - keyword tab-completion
;; - the quotes-within-quotes thing plays merry hell with
;;   paredit-doublequote, write a new doublequote function?
;; - font-lock-syntactic-keywords is obsolete since 24.1
;; - derive cg-mode from prog-mode?
;; - goto-set/list
;; - show definition of set/list-at-point in modeline
;; - send dictionary to auto-complete
;; - show section name/number in modeline

;;; Code:

(defconst cg-version "0.1.6" "Version of cg-mode")

(eval-when-compile (require 'cl))

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
  :type 'string)
(make-variable-buffer-local 'cg-extra-args)

;;;###autoload
(defcustom cg-extra-args "--trace"
  "Extra arguments sent to vislcg3 when running `cg-check'.

Buffer-local, so use `setq-default' if you want to change the
global default value. 

See also `cg-command'."
  :type 'string)
(make-variable-buffer-local 'cg-extra-args)
(setq-default cg-extra-args "--trace")

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
  :type 'string)
(make-variable-buffer-local 'cg-post-pipe)

(defconst cg-kw-set-list
  '("LIST" "SET" "TEMPLATE"
    ;; These are not sets (and don't have names after the kw) but we
    ;; have them here to make beginning-of-defun work:
    "MAPPING-PREFIX" "SOFT-DELIMITERS" "DELIMITERS")
  "Used for indentation, highlighting etc.; don't change without
re-evaluating `cg-kw-re' (or all of cg.el).")
(defconst cg-kw-set-re (regexp-opt cg-kw-set-list))

(defconst cg-kw-rule-list
  '("SUBSTITUTE"
    "IFF"
    "ADDCOHORT" "REMCOHORT"
    "COPY"
    "MOVE" "SWITCH"
    "EXTERNAL" "DELIMIT"
    "MAP"    "ADD"
    "UNMAP"
    "SELECT" "REMOVE"
    "SETPARENT"    "SETCHILD"
    "ADDRELATION"  "REMRELATION"  "SETRELATION"
    "ADDRELATIONS" "REMRELATIONS" "SETRELATIONS"
    "SETVARIABLE"  "REMVARIABLE"
    "APPEND")
  "Used for indentation, highlighting etc.; don't change without
re-evaluating `cg-kw-re' (or all of cg.el)." )
(defconst cg-kw-rule-re (regexp-opt cg-kw-rule-list))
(defconst cg-kw-re (regexp-opt (append cg-kw-set-list cg-kw-rule-list)))

(defconst cg-kw-rule-flags '("NEAREST"
			     "ALLOWLOOP"
			     "DELAYED"
			     "IMMEDIATE"
			     "LOOKDELETED"
			     "LOOKDELAYED"
			     "UNSAFE" ; 
			     "SAFE"
			     "REMEMBERX"
			     "RESETX"
			     "KEEPORDER"
			     "VARYORDER"
			     "ENCL_INNER"
			     "ENCL_OUTER"
			     "ENCL_FINAL"
			     "ENCL_ANY"
			     "ALLOWCROSS"
			     "WITHCHILD"
			     "NOCHILD"
			     "ITERATE"
			     "NOITERATE"
			     "UNMAPLAST"
			     "REVERSE"
			     "SUB"
			     "OUTPUT")
  "Used for highlighting, from
  http://visl.sdu.dk/svn/visl/tools/vislcg3/trunk/src/Strings.cpp
  Don't change without re-evaluating the file.")
(defconst cg-kw-context-flags '("NOT"
				"NEGATE"
				"NONE"
				"LINK"
				"BARRIER"
				"CBARRIER"
				"OR"
				"TARGET"
				"IF"
				"AFTER"
				"BEFORE"
				"WITH"
				"TO")
  "Used for highlighting; Don't change without re-evaluating the
  file.")
(defconst cg-kw-flags-re (regexp-opt (append cg-kw-rule-flags cg-kw-context-flags)))


;;;###autoload
(defcustom cg-indentation 8
  "The width for indentation in Constraint Grammar mode."
  :type 'integer)
(put 'cg-indentation 'safe-local-variable 'integerp)

(defconst cg-font-lock-keywords-1
  (let ((<word>? "\\(?:\"<[^>]+>\"\\)?"))
    `((,(format "^[ \t]*\\(%s\\)[ \t]+\\(\\(\\sw\\|\\s_\\)+\\)" cg-kw-set-re)
       (1 font-lock-keyword-face)
       (2 font-lock-variable-name-face))
      ("^[ \t]*\\(MAPPING-PREFIX\\|DELIMITERS\\|SOFT-DELIMITERS\\)"
       1 font-lock-keyword-face)
      ("^[ \t]*\\(SECTION\\|AFTER-SECTIONS\\|BEFORE-SECTIONS\\|MAPPINGS\\|CONSTRAINTS\\|CORRECTIONS\\)"
       1 font-lock-warning-face)
      (,(format "^[ \t]*%s[ \t]*\\(%s\\)\\(\\(:\\(\\s_\\|\\sw\\)+\\)?\\)" <word>? cg-kw-rule-re)
       (1 font-lock-keyword-face)
       (2 font-lock-variable-name-face))
      ("[ \t\n]\\([+-]\\)[ \t\n]"
       1 font-lock-function-name-face)))
  "Subdued level highlighting for CG mode.")

(defconst cg-font-lock-keywords-2
  (append cg-font-lock-keywords-1
          `(("\\<\\(&&\\(\\s_\\|\\sw\\)+\\)\\>"
             (1 font-lock-variable-name-face))
            ("\\<\\(\\$\\$\\(\\s_\\|\\sw\\)+\\)\\>"
             (1 font-lock-variable-name-face))
            (,(format "\\<\\(%s\\|[psc][lroOxX]*\\)\\>" cg-kw-flags-re)
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

(defun cg-syntax-at-pos ()
  (let ((ppss (syntax-ppss)))
    (cond
     ((nth 8 ppss) (if (nth 4 ppss) 'comment 'string))
     ((nth 1 ppss) 'paren))))

(defun cg-beginning-of-defun ()
  (re-search-backward defun-prompt-regexp nil 'noerror)
  (while (cg-syntax-at-pos)
    (re-search-backward defun-prompt-regexp nil 'noerror))
  (re-search-backward "\"<[^\"]>\"" (line-beginning-position) 'noerror))

(defun cg-end-of-defun ()
  (and (search-forward ";")
       (re-search-forward defun-prompt-regexp nil 'noerror)
       (goto-char (match-beginning 0)))
  (while (cg-syntax-at-pos)
    (and (search-forward ";")
	 (re-search-forward defun-prompt-regexp nil 'noerror)
	 (goto-char (match-beginning 0))))
  (re-search-backward "\"<[^\"]>\"" (line-beginning-position) 'noerror))

;;;###autoload
(defun cg-mode ()
  "Major mode for editing Constraint Grammar files.

CG-mode provides the following specific keyboard key bindings:

\\{cg-mode-map}"
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
  (set (make-local-variable 'defun-prompt-regexp) cg-kw-re)
  (set (make-local-variable 'beginning-of-defun-function) #'cg-beginning-of-defun)
  (set (make-local-variable 'end-of-defun-function) #'cg-end-of-defun)
  (setq indent-line-function #'cg-indent-line)
  (easy-mmode-pretty-mode-name 'cg-mode " cg")
  (when font-lock-mode
    (setq font-lock-set-defaults nil)
    (font-lock-set-defaults)
    (font-lock-fontify-buffer))
  (add-hook 'after-change-functions #'cg-after-change nil 'buffer-local)
  (let ((buf (current-buffer)))
    (run-with-idle-timer 1 'repeat 'cg-output-hl buf))
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
  "CG mode face used to highlight troublesome strings with unescaped quotes in them.")




;;; Indentation

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
                      (re-search-backward cg-kw-re nil 'noerror))))
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

(defvar cg--occur-history nil)
(defvar cg--occur-prefix-history nil)
(defvar cg--goto-history nil)

(defun cg-permute (input)
  "From http://www.emacswiki.org/emacs/StringPermutations"
  (require 'cl)	; TODO: (require 'cl-lib) for whole file when 24.3 in distros
  (if (null input)
      (list input)
    (mapcan (lambda (elt)
              (mapcan (lambda (p)
                        (list (cons elt p)))
                      (cg-permute (remove* elt input :count 1))))
            input)))

(defun cg-read-arg (prompt history &optional default)
  (let* ((default (or default (car history)))
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
                        cg--occur-prefix-history))
                     (cg-read-arg
                      "Words to occur between LIST/SET and ="
                      cg--occur-history)))
  (let* ((words-perm (cg-permute (split-string words " " 'omitnulls)))
         ;; can't use regex-opt because we need .* between the words
         (perm-disj (mapconcat (lambda (word)
                                 (mapconcat 'identity word ".*"))
                               words-perm
                               "\\|")))
    (setq cg--occur-history (cons words cg--occur-history))
    (setq cg--occur-prefix-history (cons prefix cg--occur-prefix-history))
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
least -- selects the whole string \"SELECT:1022:rulename\")."
  (interactive (list (when current-prefix-arg
                       (cg-read-arg "Paste rule info from --trace here: "
                                    cg--goto-history))))
  (let ((errmsg (if input (concat "Unrecognised rule/trace format: " input)
                  "X clipboard does not seem to contain vislcg3 --trace rule info"))
        (rule (or input (with-temp-buffer
                          (yank)
                          (buffer-substring-no-properties (point-min)(point-max))))))
    (if (string-match
         "\\(\\(select\\|iff\\|remove\\|map\\|addcohort\\|remcohort\\|switch\\|copy\\|add\\|substitute\\):\\)?\\([0-9]+\\)"
         rule)
        (progn (goto-char (point-min))
	       (forward-line (1- (string-to-number (match-string 3 rule))))
               (setq cg--goto-history (cons rule cg--goto-history)))
      (message errmsg))))



;;; "Flycheck" ----------------------------------------------------------------
(require 'compile)

(defvar cg--file nil
  "Which CG file the `cg-output-mode' (and `cg--check-cache-buffer')
buffer corresponds to.")
(make-variable-buffer-local 'cg--file)
(defvar cg--tmp nil     ; TODO: could use cg--file iff buffer-modified-p
  "Which temporary file was sent in lieu of `cg--file' to
compilation (in case the buffer of `cg--file' was not saved)")
(make-variable-buffer-local 'cg--tmp)
(defvar cg--cache-in nil
  "Which input buffer the `cg--check-cache-buffer' corresponds
to.")
(make-variable-buffer-local 'cg--cache-in)
(defvar cg--cache-pre-pipe nil
  "Which pre-pipe the output of `cg--check-cache-buffer' had.")
(make-variable-buffer-local 'cg--cache-pre-pipe)

(unless (fboundp 'file-name-base)	; shim for 24.3 function
  (defun file-name-base (&optional filename)
    (let ((filename (or filename (buffer-file-name))))
      (file-name-nondirectory (file-name-sans-extension filename)))))

(defun cg-edit-input ()
  "Open a buffer to edit the input sent when running `cg-check'."
  (interactive)
  (pop-to-buffer (cg-input-buffer (buffer-file-name))))

;;;###autoload
(defcustom cg-check-do-cache t
  "If non-nil, `cg-check' caches the output of `cg-pre-pipe' (the
cache is emptied whenever you make a change in the input buffer,
or call `cg-check' from another CG file).")

(defvar cg--check-cache-buffer nil "See `cg-check-do-cache'.")

(defun cg-input-mode-bork-cache (from to len)
  "Since `cg-check' will not reuse a cache unless `cg--file' and
`cg--cache-in' match."
  (when cg--check-cache-buffer
    (with-current-buffer cg--check-cache-buffer
      (setq cg--file nil
            cg--cache-pre-pipe nil
            cg--cache-in nil))))

(defun cg-pristine-cache-buffer (file in pre-pipe)
  (with-current-buffer (setq cg--check-cache-buffer
                             (get-buffer-create "*cg-pre-cache*"))
    (widen)
    (delete-region (point-min) (point-max))
    (set (make-local-variable 'cg--file) file)
    (set (make-local-variable 'cg--cache-in) in)
    (set (make-local-variable 'cg--cache-pre-pipe) pre-pipe)
    (current-buffer)))

(defvar cg-input-mode-map (make-sparse-keymap)
  "Keymap for CG input mode.")

(define-derived-mode cg-input-mode fundamental-mode "CG-in"
  "Input for `cg-mode' buffers."
  (use-local-map cg-input-mode-map)
  (add-hook 'after-change-functions #'cg-input-mode-bork-cache nil t))


;;;###autoload
(defcustom cg-per-buffer-input nil
  "If this is non-nil, the input buffer created by
`cg-edit-input' will be specific to the CG buffer it was called
from, otherwise all CG buffers share one input buffer."
  :type 'string)

(defun cg-input-buffer (file)
  (let ((buf (get-buffer-create (concat "*CG input"
                                        (if cg-per-buffer-input
                                            (concat " for " (file-name-base file))
                                          "")
                                        "*"))))
    (with-current-buffer buf
      (cg-input-mode)
      (setq cg--file file))
    buf))

(defun cg-get-file ()
  (list cg--file))

(defconst cg-output-regexp-alist
  `((,(format "%s:\\([^ \n\t:]+\\)\\(?::[^ \n\t]+\\)?" cg-kw-rule-re)
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
  :type 'hook)

(defun cg-output-process-setup ()
  "Runs `cg-output-setup-hook' for `cg-check'. That hook is
useful for doing things like
 (setenv \"PATH\" (concat \"~/local/stuff\" (getenv \"PATH\")))"
  (run-hooks #'cg-output-setup-hook))

(defvar cg-output-comment-face  font-lock-comment-face	;compilation-info-face
  "Face name to use for comments in cg-output.")

(defvar cg-output-form-face	'compilation-error
  "Face name to use for forms in cg-output.")

(defvar cg-output-lemma-face	font-lock-string-face
  "Face name to use for lemmas in cg-output.")

(defvar cg-output-mapping-face 'bold
  "Face name to use for mapping tags in cg-output")

(defvar cg-output-mode-font-lock-keywords 
  '(("^;\\(?:[^:]* \\)"
     ;; hack alert! a colon in a tag will mess this up
     ;; (hardly matters much though)
     0 cg-output-comment-face)
    ("\"<[^>\n]+>\""
     0 cg-output-form-face)
    ("\t\\(\".*\"\\) "
     ;; easier to match "foo"bar" etc. here since it's always the first tag
     1 cg-output-lemma-face)
    ("\\_<@[^ \n]+"
     0 cg-output-mapping-face))
  "Additional things to highlight in CG output.
This gets tacked on the end of the generated expressions.")

(defvar cg--output-display-table (make-display-table)
  "Used to turn ellipses into spaces when hiding analyses.")

(defvar cg-sent-tag "\\bsent\\b"
  "When using `cg-output-hide-analyses', any cohort matching this
regex gets a newline tacked on after the wordform.")

(defvar cg-output-unhide-regex nil
  "Regular expression exempt from hiding when doing
`cg-output-hide-analyses'.")

(defvar cg--output-hiding-analyses nil
  "If non-nil, re-hide analyses after `cg-check'. Saves you from
having to re-enter the buffer and press `h' if you you want to
keep analyses hidden most of the time.")

(defvar cg--output-unhide-history nil)



(define-compilation-mode cg-output-mode "CG-out"
  "Major mode for output of Constraint Grammar compilations and
runs."
  ;; cg-output-mode-font-lock-keywords applied automagically
  (set (make-local-variable 'compilation-skip-threshold)
       1)
  (set (make-local-variable 'compilation-error-regexp-alist)
       cg-output-regexp-alist)
  (set (make-local-variable 'cg--file)
       nil)
  (set (make-local-variable 'cg--tmp)
       nil)
  (set (make-local-variable 'compilation-disable-input)
       nil)
  ;; compilation-directory-matcher can't be nil, so we set it to a regexp that
  ;; can never match.
  (set (make-local-variable 'compilation-directory-matcher)
       '("\\`a\\`"))
  (set (make-local-variable 'compilation-process-setup-function)
       #'cg-output-process-setup)
  ;; (add-hook 'compilation-filter-hook 'cg-output-filter nil t) ; TODO: nab grep code mogrifying bash colours
  ;; We send text to stdin:
  (set (make-local-variable 'compilation-disable-input)
       nil)
  (set (make-local-variable 'compilation-finish-functions)
       (list #'cg-check-finish-function))
  (modify-syntax-entry ?§ "_")
  (modify-syntax-entry ?@ "_")
  ;; For cg-output-hide-analyses:
  (add-to-invisibility-spec '(cg-output . t))
  ;; snatched from `org-mode':
  (when (and (fboundp 'set-display-table-slot) (boundp 'buffer-display-table)
	     (fboundp 'make-glyph-code))
    (set-display-table-slot cg--output-display-table
			    4
			    (vconcat (make-glyph-code " ")))
    (setq buffer-display-table cg--output-display-table)))



(defun cg-output-remove-overlay (overlay)
  (remove-overlays (overlay-start overlay) (overlay-end overlay) 'invisible 'cg-output))

(defun cg-output-hide-region (from to)
  (remove-overlays from to 'invisible 'cg-output)
  (let ((o (make-overlay from to nil)))
    (overlay-put o 'evaporate t)
    (overlay-put o 'invisible 'cg-output)
    (overlay-put o 'isearch-open-invisible 'cg-output-remove-overlay)))

(defun cg-output-show-all ()
  "Undoes the effect of `cg-output-hide-analyses'."
  (interactive)
  (setq cg--output-hiding-analyses nil)
  (remove-overlays (point-min) (point-max) 'invisible 'cg-output))

(defun cg-output-hide-analyses ()
  "Hides all analyses, turning the CG format back into input
text (more or less). You can still isearch through the text for
tags, REMOVE/SELECT keywords etc.

Call `cg-output-set-unhide' to set a regex which will be exempt
from hiding. Call `cg-output-show-all' to turn off all hiding."
  (interactive)
  (setq cg--output-hiding-analyses t)
  (lexical-let (last)
    (save-excursion
      (goto-char (point-min))
      (while (re-search-forward "^\"<.*>\"$" nil 'noerror)
	(let ((line-beg (match-beginning 0))
	      (line-end (match-end 0)))
	  (cg-output-hide-region line-beg (+ line-beg 2)) ; "<
	  (cg-output-hide-region (- line-end 2) line-end) ; >"
	  (when last
	    (if (save-excursion (re-search-backward cg-sent-tag last 'noerror))
		(cg-output-hide-region last (- line-beg 1))	; show newline
	      (cg-output-hide-region last line-beg))) ; hide newline too
	  (setq last line-end)))
      (goto-char last)
      (when (re-search-forward "^[^\t\"]" nil 'noerror)
	(cg-output-hide-region last (match-beginning 0)))))
  
  (when cg-output-unhide-regex
    (cg-output-unhide-some cg-output-unhide-regex)))

(defun cg-output-unhide-some (needle)
  (save-excursion
    (goto-char (point-min))
    (while (re-search-forward needle nil 'noerror)
      (mapc (lambda (o)
	      (when (eq 'cg-output (overlay-get o 'invisible))
		(remove-overlays (overlay-start o) (overlay-end o)
				 'invisible 'cg-output)))
	    (overlays-at (match-beginning 0))))))

(defun cg-output-set-unhide (needle)
  "Set some exeption to `cg-output-hide-analyses'. This is saved
and reused whenever `cg-output-hide-analyses' is called."
  (interactive (list (cg-read-arg
		      "Regex to unhide, or empty to hide all"
		      cg--output-unhide-history
		      "")))
  (if (equal needle "")
      (setq cg-output-unhide-regex nil)
    (setq cg-output-unhide-regex needle)
    (setq cg--output-unhide-history (cons needle cg--output-unhide-history)))
  (cg-output-hide-analyses))

;;; TODO: 
(defun cg-output-toggle-analyses ()
  "Hide or show analyses from output. See
`cg-output-hide-analyses'."
  (interactive)
  (if cg--output-hiding-analyses
      (cg-output-show-all)
    (cg-output-hide-analyses)))



;;;###autoload
(defcustom cg-check-after-change nil
  "If non-nil, run `cg-check' on grammar after each change to the
buffer.")

;;;###autoload
(defcustom cg-check-after-change-secs 1
  "Minimum seconds between each `cg-check' after a change to a CG
buffer (so 0 is after each change)."
  :type 'integer)

(defvar cg--after-change-timer nil)
(defun cg-after-change (from to len)
  (when (and cg-check-after-change
             (not (member cg--after-change-timer timer-list)))
    (setq
     cg--after-change-timer
     (run-at-time
      cg-check-after-change-secs
      nil
      (lambda ()
        (unless (cg-output-running)
	  (with-demoted-errors (cg-check))))))))

(defun cg-output-hl (cg-buffer)
  (when (eq (current-buffer) cg-buffer)
    (let* ((sym (symbol-at-point))
	   (sym-re (concat "[ \"]\\("
			   (regexp-quote (symbol-name sym))
			   "\\)\\([\" ]\\|$\\)")))
      ;; TODO: make regexp-opts of the LIST definitions and search
      ;; those as well?
      (with-current-buffer (cg-output-buffer)
	(when (and sym
		   (get-buffer-window)
		   (not (cg-output-running)))
	  (remove-overlays (point-min) (point-max) 'face 'lazy-highlight)
	  (goto-char (point-min))
	  (while (re-search-forward sym-re nil 'noerror)
	    (overlay-put (make-overlay (match-beginning 1) (match-end 1))
			 'face 'lazy-highlight)))))))

(defun cg-output-running ()
  (let ((proc (get-buffer-process (cg-output-buffer))))
    (and proc (eq (process-status proc) 'run))))

(defun cg-output-buffer-name (mode)
  (if (equal mode "cg-output")
      (concat "*CG output for " (file-name-base cg--file) "*")
    (error "Unexpected mode %S" mode)))

(defun cg-output-buffer ()
  (let ((cg--file (if (eq major-mode 'cg-mode)
		      (buffer-file-name)
		    cg--file)))
    (get-buffer-create (compilation-buffer-name
			"cg-output"
			'cg-output-mode
			'cg-output-buffer-name))))

(defun cg-end-process (proc &optional string)
  "End `proc', optionally first sending in `string'."
  (when string
    (process-send-string proc string))
  (process-send-string proc "\n")
  (process-send-eof proc))

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
  (lexical-let*
      ((file (buffer-file-name))
       (tmp (make-temp-file "cg."))
       ;; Run in a separate process buffer from cmd and post-pipe:
       (pre-pipe (if (and cg-pre-pipe (not (equal "" cg-pre-pipe)))
                     cg-pre-pipe
                   "cat"))
       ;; Tacked on to cmd, thus the |:
       (post-pipe (if (and cg-post-pipe (not (equal "" cg-post-pipe)))
                      (concat " | " cg-post-pipe)
                    ""))
       (cmd (concat
             cg-command " " cg-extra-args " --grammar " tmp
             post-pipe))
       (in (cg-input-buffer file))
       (out (progn (write-region (point-min) (point-max) tmp)
                   (compilation-start
                    cmd
                    'cg-output-mode
                    'cg-output-buffer-name))))

    (with-current-buffer out
      (setq cg--tmp tmp)
      (setq cg--file file))

    (if (and cg-check-do-cache
             (buffer-live-p cg--check-cache-buffer)
             (with-current-buffer cg--check-cache-buffer
               ;; Check that the cache is for this grammar and input:
               (and (equal cg--cache-pre-pipe pre-pipe)
                    (equal cg--file file)
                    (equal cg--cache-in in))))

        (with-current-buffer cg--check-cache-buffer
          (cg-end-process (get-buffer-process out) (buffer-string)))

      (lexical-let ((cg-proc (get-buffer-process out))
                    (pre-proc (start-process "cg-pre-pipe" "*cg-pre-pipe-output*"
                                             "/bin/bash" "-c" pre-pipe))
                    (cache-buffer (cg-pristine-cache-buffer file in pre-pipe)))
        (set-process-filter pre-proc (lambda (pre-proc string)
                                       (with-current-buffer cache-buffer
                                         (insert string))
                                       (when (eq (process-status cg-proc) 'run)
                                         (process-send-string cg-proc string))))
        (set-process-sentinel pre-proc (lambda (pre-proc string)
                                         (when (eq (process-status cg-proc) 'run)
                                           (cg-end-process cg-proc))))
        (with-current-buffer in
          (cg-end-process pre-proc (buffer-string)))))

    (display-buffer out)))

(defun cg-check-finish-function (buffer change)
  ;; Note: this makes `recompile' not work, which is why `g' is
  ;; rebound in `cg-output-mode'
  (let ((w (get-buffer-window buffer)))
    (when w
      (with-selected-window (get-buffer-window buffer)
        (scroll-up-line 4))))
  (with-current-buffer buffer
    (delete-file cg--tmp))
  (when cg--output-hiding-analyses
    (cg-output-hide-analyses)))

(defun cg-back-to-file-and-edit-input ()
  (interactive)
  (cg-back-to-file)
  (cg-edit-input))

(defun cg-back-to-file ()
  (interactive)
  (bury-buffer)
  (let* ((cg-buffer (find-buffer-visiting cg--file))
         (cg-window (get-buffer-window cg-buffer)))
    
    (if cg-window
        (select-window cg-window)
      (pop-to-buffer cg-buffer))))


(defun cg-back-to-file-and-check ()
  (interactive)
  (cg-back-to-file)
  (cg-check))


(defun cg-toggle-check-after-change ()
  (interactive)
  (setq cg-check-after-change (not cg-check-after-change))
  (message "%s after each change" (if cg-check-after-change
                                      (format "Checking CG %s seconds" cg-check-after-change-secs)
                                    "Not checking CG")))


;;; Keybindings ---------------------------------------------------------------
(define-key cg-mode-map (kbd "C-c C-o") #'cg-occur-list)
(define-key cg-mode-map (kbd "C-c g") #'cg-goto-rule)
(define-key cg-mode-map (kbd "C-c C-c") #'cg-check)
(define-key cg-mode-map (kbd "C-c C-i") #'cg-edit-input)
(define-key cg-mode-map (kbd "C-c c") #'cg-toggle-check-after-change)
(define-key cg-output-mode-map (kbd "C-c C-i") #'cg-back-to-file-and-edit-input)
(define-key cg-output-mode-map (kbd "i") #'cg-back-to-file-and-edit-input)
(define-key cg-output-mode-map (kbd "g") #'cg-back-to-file-and-check)
(define-key cg-output-mode-map (kbd "h") #'cg-output-toggle-analyses)
(define-key cg-output-mode-map (kbd "u") #'cg-output-set-unhide)
;;; TODO: C-c C-h for toggling hiding from grammar buffer? That'll
;;; shadow the old C-c C-h binding though ("help for C-c prefix")

(define-key cg-input-mode-map (kbd "C-c C-c") #'cg-back-to-file-and-check)
(define-key cg-output-mode-map (kbd "C-c C-c") #'cg-back-to-file)

(define-key cg-output-mode-map (kbd "n") 'next-error-no-select)
(define-key cg-output-mode-map (kbd "p") 'previous-error-no-select)

(define-key cg-mode-map (kbd "C-c C-n") 'next-error)
(define-key cg-mode-map (kbd "C-c C-p") 'previous-error)

;;; Turn on for .cg3 files ----------------------------------------------------
;;;###autoload
(add-to-list 'auto-mode-alist '("\\.cg3\\'" . cg-mode))
;; Tino Didriksen recommends this file suffix.

;;; Run hooks -----------------------------------------------------------------
(run-hooks #'cg-load-hook)

(provide 'cg)

;;;============================================================================

;;; cg.el ends here
