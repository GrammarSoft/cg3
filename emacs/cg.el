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
;;; Kevin Brubeck Unhammer <unhammer hos gmail punktum com>

;;; Usage:
;; (autoload 'cg-mode "/path/to/cg.el"
;;  "cg-mode is a major mode for editing Constraint Grammar files."  t)
;; ; whatever file ending you use:
;; (add-to-list 'auto-mode-alist '("\\.cg\\'" . cg-mode))

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
;;  - the quotes-within-quotes thing plays merry hell with
;;    paredit-doublequote, write a new doublequote function? 

(defconst cg-version "2013-03-13" "Version of cg-mode") 

;;;============================================================================
;;;
;;; Define the formal stuff for a major mode named cg.
;;;

(defvar cg-mode-map (make-sparse-keymap)
  "Keymap for cg minor mode.")

(defgroup cg nil
  "Major mode for editing VISL CG-3 Constraint Grammar files."
  :tag "CG"
  :group 'languages)

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
      (,(concat "^[ \t]*" <word>? "[ \t]*\\(SETPARENT\\|SETCHILD\\|ADDRELATIONS?\\|SETRELATIONS?\\|REMRELATIONS?\\|SUBSTITUTE\\|ADDCOHORT\\|REMCOHORT\\|MAP\\|IFF\\|ADD\\|SELECT\\|REMOVE\\)\\(\\(:\\(\\s_\\|\\sw\\)+\\)?\\)")
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
	    ("\\<\\(NOT\\|NEGATE\\|NONE\\|LINK\\|BARRIER\\|CBARRIER\\|OR\\|TARGET\\|IF\\|AFTER\\|TO\\|[psc][oO]?\\)\\>"
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
	  nil				; CASE-FOLD
	  ((?/ . "w") (?~ . "w") (?. . "w") (?- . "w") (?_ . "w"))
	  nil ;	  beginning-of-line		; SYNTAX-BEGIN
	  (font-lock-syntactic-keywords . cg-font-lock-syntactic-keywords)
	  (font-lock-syntactic-face-function . cg-font-lock-syntactic-face-function)))
  (make-local-variable 'cg-mode-syntax-table)
  (set-syntax-table cg-mode-syntax-table)
  (set (make-local-variable 'parse-sexp-ignore-comments) t)
  (set (make-local-variable 'parse-sexp-lookup-properties) t)
  (setq indent-line-function 'cg-indent-line)
  (easy-mmode-pretty-mode-name 'cg-mode " cg")
  (when font-lock-mode
    (setq font-lock-set-defaults nil)
    (font-lock-set-defaults)
    (font-lock-fontify-buffer))
  (run-mode-hooks 'cg-mode-hook))


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
  (cond ((nth 3 state) font-lock-string-face)
 	(t font-lock-comment-face)))


;;; Indentation 

(defvar cg-kw-list
  '("SUBSTITUTE" "IFF"
    "ADDCOHORT" "REMCOHORT"
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
    (mapcan '(lambda (elt)
		(mapcan '(lambda (p)
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
	 (perm-disj (mapconcat '(lambda (word)
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
	 "\\(\\(select\\|iff\\|remove\\|map\\|addcohort\\|remcohort\\|add\\|substitute\\):\\)?\\([0-9]+\\)"
	 rule)
	(progn (goto-line (string-to-number (match-string 3 rule)))
	       (setq cg-goto-history (cons rule cg-goto-history)))
      (message errmsg))))


;;; Keybindings --------------------------------------------------------------
(define-key cg-mode-map (kbd "C-c C-o") 'cg-occur-list)
(define-key cg-mode-map (kbd "C-c C-c") 'cg-goto-rule)

;;; Run hooks -----------------------------------------------------------------
(run-hooks 'cg-load-hook)

(provide 'cg)

;;;============================================================================

;;; cg.el ends here
