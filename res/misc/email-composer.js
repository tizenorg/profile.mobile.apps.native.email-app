/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

var Direction = {
	"LEFT": 0,
	"RIGHT": 1,
	"TOP": 2,
	"BOTTOM": 3,
	"LEFT_TOP": 4,
	"RIGHT_TOP": 5,
	"LEFT_BOTTOM": 6,
	"RIGHT_BOTTOM": 7,
	"MOVE": 8
};

var FontSize = {
	"1": 10,
	"2": 14,
	"3": 16,
	"4": 18,
	"5": 24,
	"6": 32,
	"7": 48
};

/* Define names */
var G_ENABLE_DEBUG_LOGS = false;

var G_NAME_NEW_MESSAGE = 'DIV_NewMessage';
var G_NAME_ORG_MESSAGE = 'DIV_OrgMessage';
var G_NAME_ORG_MESSAGE_BAR = 'DIV_OrgMessageBar';
var G_NAME_ORG_MESSAGE_BAR_CHECKBOX = 'DIV_OrgMessageBarCheckbox';
var G_NAME_ORG_MESSAGE_BAR_TEXT = "DIV_OrgMessageBarText";
var G_NAME_ORG_MESSAGE_BAR_ROW = "DIV_OrgMessageBarRow";

var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKED = "css-originalMessageBar-checked";
var G_NAME_CSS_ORG_MESSAGE_BAR_UNCHECKED = "css-originalMessageBar-unchecked";
var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED = "css-originalMessageBarCheckbox-checked";
var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_UNCHECKED = "css-originalMessageBarCheckbox-unchecked";

var G_VAL_PIXEL_RATIO = window.devicePixelRatio;
var G_VAL_CSS_SCALE_RATIO = (1.0 / G_VAL_PIXEL_RATIO);

var G_VAL_DEFAULT_FONT_COLOR = "rgb(0, 0, 0)";
var G_VAL_DEFAULT_BG_COLOR = "rgb(255, 255, 255)";
var G_VAL_DEFAULT_FONT_SIZE = "16px";
var G_VAL_TEXT_NODE_TYPE = 3;
var G_VAL_ORG_MESSAGE_BAR_TEXT_ALIGN = "display:table-cell; vertical-align: middle; padding-left:10px; padding-top:10px; padding-bottom:10px; width:auto;";
var G_VAL_ORG_MESSAGE_BAR_TEXT_SIZE = 18;

var G_VAL_REMOVE_INVALID_IMAGES = false;

var G_VAL_IMAGE_INITIZL_SIZE = 0.8;
var G_VAL_IMAGE_STYLE = "margin: 4px 0px; padding: 0px; display: block;";

var G_VAL_IMAGE_OVERLAY_COLOR = "rgba(0, 0, 0, 0.2)";
var G_VAL_IMAGE_OUTLINE_COLOR = "rgb(0, 191, 230)";
var G_VAL_IMAGE_HANDLE_IMG_PATH = "images/composer_icon/email_crop_handler_padded.png";
var G_VAL_IMAGE_OUTLINE_WIDTH = 4;
var G_VAL_IMAGE_HANDLE_SIZE = 32;
var G_VAL_IMAGE_HANDLE_TOUCH_RADIUS = G_VAL_IMAGE_HANDLE_SIZE;
var G_VAL_IMAGE_MIN_SIZE = (2 * G_VAL_IMAGE_HANDLE_SIZE);

var G_VAL_LONG_TAP_DELAY_MS = 500.0;
var G_VAL_MIN_HIT_TEST_INTERVAL_MS = 150.0;

var g_isTouchStartOnOriginalMessageBar = false;
var g_isTouchMoveOnOriginalMessageBar = false;

var g_selectionRange = null;

var g_curFontSize = 0;
var g_curBold = 0;
var g_curUnderline = 0;
var g_curItalic = 0;
var g_curFontColor = "";
var g_curBgColor = "";
var g_curOrderedList = 0;
var g_curUnorderedList = 0;

var g_lastRange = null;
var g_lastRangeIsForward = false;

var g_scrollTop = 0;
var g_frameRequestId = 0;

var g_imageLayer = null;

var utils = {
	/** @param {Range} newRange */
	setSelectionFromRange: function (newRange) {
		var selection = window.getSelection();

		if (selection.rangeCount > 0) {
			if (newRange && (selection.rangeCount == 1)) {
				var curRange = selection.getRangeAt(0);
				var startPointCompareRes = curRange.compareBoundaryPoints(Range.START_TO_START, newRange);
				var endPointCompareRes = curRange.compareBoundaryPoints(Range.END_TO_END, newRange);
				if (startPointCompareRes == 0 && endPointCompareRes == 0) {
					return;
				}
			}
			selection.removeAllRanges();
		}

		if (newRange) {
			selection.addRange(newRange);
		} else {
			document.activeElement.blur();
		}
	},

	clearSelection: function () {
		utils.setSelectionFromRange(null);
	},

	/** @param {HTMLElement} element */
	getBoundingPageRect: function (element) {
		var htmlElement = document.documentElement;
		var htmlClientRect = htmlElement.getBoundingClientRect();
		var elementClientRect = element.getBoundingClientRect();
		return {
			left: (elementClientRect.left - htmlClientRect.left + htmlElement.offsetLeft),
			top: (elementClientRect.top - htmlClientRect.top + htmlElement.offsetTop),
			width: elementClientRect.width,
			height: elementClientRect.height
		};
	},

	/** @param {HTMLElement} element */
	isElementEditable: function (element) {
		var parent = element.parentElement;
		while (parent) {
			if (parent.contentEditable === "true") {
				return true;
			}
			parent = parent.parentElement;
		}
		return false;
	},

	/** @param {TouchEvent} touchEvent
	 *  @param {number} touchId */
	findChangedTouchById: function (touchEvent, touchId) {
		if (touchId < 0) {
			return null;
		}
		var changedTouches = touchEvent.changedTouches;
		var n = changedTouches.length;
		for (var i = 0; i < n; ++i) {
			var changedTouch = changedTouches[i];
			if (changedTouch.identifier === touchId) {
				return changedTouch;
			}
		}
		return null;
	},

	/** @param {string} event */
	sendUserEvent: function (event) {
		console.log("EMAIL_EWK_EVENT: " + event);
	},

	log: function (msg) {
		console.log("EMAIL_EWK_LOG: " + msg);
	}
};

if (!G_ENABLE_DEBUG_LOGS) {
	utils.log = function () { };
}

utils.log("==> screen.width:" + screen.width);
utils.log("==> window.innerWidth:" + window.innerWidth);
utils.log("==> window.devicePixelRatio:" + window.devicePixelRatio);

function CaretPos(top, bottom, isCollapsed) {
	this.top = Math.round(top);
	this.bottom = Math.round(bottom);
	this.isCollapsed = isCollapsed;

	this.toString = function () {
		return this.top + " " + this.bottom + " " + this.isCollapsed;
	};
}

/* Related to the behavior of Original Message Bar */
function OnTouchStart_OriginalMessageBar(ev) {
	utils.log(arguments.callee.name + "()");
	g_isTouchStartOnOriginalMessageBar = true;
}

function OnTouchMove_OriginalMessageBar(ev) {
	utils.log(arguments.callee.name + "()");
	if (!g_isTouchStartOnOriginalMessageBar) {
		return;
	}
	g_isTouchMoveOnOriginalMessageBar = true;
}

function OnTouchEnd_OriginalMessageBar(ev) {
	utils.log(arguments.callee.name + "()");
	if (g_isTouchStartOnOriginalMessageBar && !g_isTouchMoveOnOriginalMessageBar) {
		utils.log("clicked!");
		ev.stopPropagation();
		ev.preventDefault();

		SwitchOriginalMessageState();
	}
	g_isTouchStartOnOriginalMessageBar = false;
	g_isTouchMoveOnOriginalMessageBar = false;
}

function OnkeyDown_Checkbox(obj, event) {
	utils.log(arguments.callee.name + "()");
	if (event.keyCode == 13) { // Enter key
		utils.log(arguments.callee.name + ": [enter]");
		event.preventDefault();
		SwitchOriginalMessageState();
	} else if (event.keyCode == 32) { // Spacebar
		utils.log(arguments.callee.name + ": [spacebar]");
		event.preventDefault();
		SwitchOriginalMessageState();
	} else {
		utils.log(arguments.callee.name + ": [" + event + " / " + event.keyCode + "]");
	}
}

function SwitchOriginalMessageState() {
	utils.log(arguments.callee.name + "()");
	var originalMessage = document.getElementById(G_NAME_ORG_MESSAGE);
	var originalMessageBar = document.getElementById(G_NAME_ORG_MESSAGE_BAR);
	var originalMessageBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);

	if (originalMessageBarCheckbox.className == G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED) {
		originalMessageBarCheckbox.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_UNCHECKED;
		originalMessageBar.className = G_NAME_CSS_ORG_MESSAGE_BAR_UNCHECKED;
		utils.sendUserEvent("checkbox-clicked:0");
		originalMessage.style.display = "none";
	} else {
		originalMessageBarCheckbox.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED;
		originalMessageBar.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKED;
		utils.sendUserEvent("checkbox-clicked:1");
		originalMessage.style.display = "block";
	}
}

/* Related to the behavior of focus */
function OnFocus_NewMessageDIV(ev) {
	utils.log(arguments.callee.name + "()");
	utils.sendUserEvent("new-message-div-focused");
}

function OnFocus_OrgMessageDIV(ev) {
	utils.log(arguments.callee.name + "()");
	utils.sendUserEvent("org-message-div-focused");
}

/* Related to the behavior of etc... */
function OnNodeInserted(obj, event) {
	if (event.target.tagName === "DIV") {
		event.target.setAttribute('dir', 'auto');
	}
}

function OnKeyDown(obj, event) {
	utils.log(arguments.callee.name + "()");
	if (event.keyCode == 9) { // Tab Key
		utils.log(arguments.callee.name + ": [tab]");
		event.preventDefault();
		document.execCommand('insertHTML', false, "\u00A0\u00A0\u00A0\u00A0");
	} else if (event.ctrlKey && (event.keyCode == 89)) { // 'ctrl+y' Key
		utils.log(arguments.callee.name + ": [ctrl+y]");
		document.execCommand('Redo', false, null);
	} else if (event.ctrlKey && (event.keyCode == 90)) { // 'ctrl+z' Key
		utils.log(arguments.callee.name + ": [ctrl+z]");
		document.execCommand('Undo', false, null);
	} else if ((event.keyCode == 13) || (event.keyCode == 8)) { // Enter, Backspace
		utils.log(arguments.callee.name + ": [special key]");
		setTimeout(function () { NotifyCaretPosChange(false); }, 0);
	} else {
		utils.log(arguments.callee.name + ": [" + event + " / " + event.keyCode + "]");
	}
}

function NotifyCaretPosChange(force) {
	var caretPos = GetCaretPosition(force);
	if (caretPos) {
		utils.sendUserEvent("caret-pos-changed:" + caretPos);
	}
}

/* Related to the behavior of Image conrol */
function OnPaste(obj, ev) {
	utils.log(arguments.callee.name + "()");
	var imageUrlNo = -1;
	if (ev.clipboardData.types) {
		var i = 0;
		while (i < ev.clipboardData.types.length) {
			var key = ev.clipboardData.types[i];
			//var val = ev.clipboardData.getData(key);
			//utils.log((i + 1) + ': ' + key + ' - ' + val);

			if (key == "Image") {
				imageUrlNo = i;
				break;
			}
			i++;
		}
	}
	//utils.log("Image No: " + imageUrlNo);

	if (imageUrlNo == -1) {
		// P140612-05478: Due to performance issue on webkit pasting and rendering a large amount of text, we need to set the limit of the text will be pasted. (100K on GalaxyS5)
		var elements = document.getElementsByTagName("body");
		var bodyLength = elements[0].innerHTML.length;
		var pasteLength = ev.clipboardData.getData(ev.clipboardData.types[0]).length;
		utils.log("Length: body:(" + bodyLength + "), paste:(" + pasteLength + ")");

		if ((bodyLength + pasteLength) >= 102400) {
			utils.log("Exceeded 100K!!");
			utils.sendUserEvent("max-size-exceeded");
			ev.preventDefault();
			ev.stopPropagation();
		}
		return;
	}
}

function ImageLayer() {

	this.cancel = function () {
		candidateElement_ = null;
	}

	/** @type {HTMLElement} */
	var element_ = null;
	/** @type {HTMLElement} */
	var candidateElement_ = null;
	/** @type {HTMLElement} */
	var targetElement_ = null;

	var activeTouchId_ = -1;
	var touchStartTime_ = 0;

	var isDragging_ = false;
	var dragX_ = 0;
	var dragY_ = 0;
	var startTouchPageX_ = 0;
	var startTouchPageY_ = 0;
	var startElementLeft_ = 0;
	var startElementTop_ = 0;
	var startElementWidth_ = 0;
	var startElementHeight_ = 0;
	var startElementAspect_ = 0;

	var lastHitTestTime_ = 0;

	function initialize() {
		element_ = document.createElement("DIV");
		element_.style.cssText = "position: absolute; pointer-events: none; display: none;";
		element_.className = "email-composer-image-layer";
		document.body.appendChild(element_);

		var overlay = document.createElement("DIV");
		overlay.style.cssText = "position: absolute; pointer-events: none; width: 100%; height: 100%; " +
			"background-color: " + G_VAL_IMAGE_OVERLAY_COLOR + "; outline: " +
			G_VAL_IMAGE_OUTLINE_WIDTH + "px solid " + G_VAL_IMAGE_OUTLINE_COLOR +
			"; outline-offset: " + (-G_VAL_IMAGE_OUTLINE_WIDTH / 2.0) + "px;";
		element_.appendChild(overlay);

		var baseDotStyle = "position: absolute; pointer-events: none; " +
			"width: " + G_VAL_IMAGE_HANDLE_SIZE + "px; height: " + G_VAL_IMAGE_HANDLE_SIZE + "px; " +
			"margin: " + (-G_VAL_IMAGE_HANDLE_SIZE / 2.0) + "px; background-color: " + G_VAL_IMAGE_OUTLINE_COLOR +
			"; -webkit-mask-image: url(" + G_VAL_IMAGE_HANDLE_IMG_PATH + "); -webkit-mask-size: cover; ";

		for (var i = 0; i < 9; ++i) {
			if (i === 4) {
				continue;
			}
			var dot = document.createElement("DIV");
			dot.style.cssText = baseDotStyle +
				"left: " + (i % 3 * 50) + "%; top: " + (Math.floor(i / 3) * 50) + "%;";
			element_.appendChild(dot);
		}
	}

	initialize();

	function activate() {
		targetElement_ = candidateElement_;
		candidateElement_ = null;

		element_.style.display = "initial";

		updatePosition();
	}

	function deactivate() {
		targetElement_ = null;

		element_.style.display = "none";
	}

	function updatePosition() {
		var targetClientRect = targetElement_.getBoundingClientRect();
		/** @type {ClientRect} */
		var offsetClientRect = null;

		if (window.getComputedStyle(document.body).position !== "static") {
			offsetClientRect = document.body.getBoundingClientRect();
		} else {
			offsetClientRect = document.documentElement.getBoundingClientRect();
		}

		element_.style.left = (targetClientRect.left - offsetClientRect.left) + "px";
		element_.style.top = (targetClientRect.top - offsetClientRect.top) + "px";
		element_.style.width = targetClientRect.width + "px";
		element_.style.height = targetClientRect.height + "px";
	}

	/** @param {Touch} touch */
	function startDrag(touch) {
		var touchPageX = touch.pageX;
		var touchPageY = touch.pageY;

		var elementPageRect = utils.getBoundingPageRect(element_);
		var elementPageX1 = elementPageRect.left;
		var elementPageX3 = elementPageX1 + elementPageRect.width;
		var elementPageY1 = elementPageRect.top;
		var elementPageY3 = elementPageY1 + elementPageRect.height;

		if ((touchPageX < elementPageX1 - G_VAL_IMAGE_HANDLE_TOUCH_RADIUS) ||
			(touchPageX >= elementPageX3 + G_VAL_IMAGE_HANDLE_TOUCH_RADIUS) ||
			(touchPageY < elementPageY1 - G_VAL_IMAGE_HANDLE_TOUCH_RADIUS) ||
			(touchPageY >= elementPageY3 + G_VAL_IMAGE_HANDLE_TOUCH_RADIUS)) {
			return false;
		}

		var dx1 = Math.abs(touchPageX - elementPageX1);
		var dx2 = Math.abs(touchPageX - (elementPageX1 + elementPageX3) / 2);
		var dx3 = Math.abs(touchPageX - elementPageX3);
		var dy1 = Math.abs(touchPageY - elementPageY1);
		var dy2 = Math.abs(touchPageY - (elementPageY1 + elementPageY3) / 2);
		var dy3 = Math.abs(touchPageY - elementPageY3);

		dragX_ = (dx1 < dx2 ? (dx1 < dx2 ? (dx1 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 1 : 0) :
			(dx3 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 3 : 0)) :
			(dx2 < dx3 ? (dx2 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 2 : 0) :
				(dx3 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 3 : 0)));

		dragY_ = (dy1 < dy2 ? (dy1 < dy2 ? (dy1 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 1 : 0) :
			(dy3 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 3 : 0)) :
			(dy2 < dy3 ? (dy2 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 2 : 0) :
				(dy3 <= G_VAL_IMAGE_HANDLE_TOUCH_RADIUS ? 3 : 0)));

		if ((dragX_ === 0) || (dragY_ === 0) || ((dragX_ === 2) && (dragY_ === 2))) {
			if ((touchPageX >= elementPageX1) && (touchPageX < elementPageX3) &&
				(touchPageY >= elementPageY1) && (touchPageY < elementPageY3)) {
				dragX_ = 2;
				dragY_ = 2;
			} else {
				return false;
			}
		}

		startTouchPageX_ = touchPageX;
		startTouchPageY_ = touchPageY;
		startElementLeft_ = parseFloat(element_.style.left);
		startElementTop_ = parseFloat(element_.style.top);
		startElementWidth_ = elementPageRect.width;
		startElementHeight_ = elementPageRect.height;
		startElementAspect_ = (startElementWidth_ / startElementHeight_);

		isDragging_ = true;

		return true;
	}

	/** @param {Touch} touch */
	function doDrag(touch) {
		var touchDX = (touch.pageX - startTouchPageX_);
		var touchDY = (touch.pageY - startTouchPageY_);

		var newElementWidth = 0;
		var newElementHeight = 0;
		var newElementAspect = 0;

		if (dragX_ === 1) {
			newElementWidth = (startElementWidth_ - touchDX);
		} else if (dragX_ === 3) {
			newElementWidth = (startElementWidth_ + touchDX);
		}

		if (dragY_ === 1) {
			newElementHeight = (startElementHeight_ - touchDY);
		} else if (dragY_ === 3) {
			newElementHeight = (startElementHeight_ + touchDY);
		}

		if (newElementWidth && (newElementWidth < G_VAL_IMAGE_MIN_SIZE)) {
			newElementWidth = G_VAL_IMAGE_MIN_SIZE;
		}
		if (newElementHeight && (newElementHeight < G_VAL_IMAGE_MIN_SIZE)) {
			newElementHeight = G_VAL_IMAGE_MIN_SIZE;
		}

		if ((newElementWidth !== 0) && (newElementHeight !== 0)) {
			newElementAspect = (newElementWidth / newElementHeight);
			if (newElementAspect > startElementAspect_) {
				newElementHeight = (newElementWidth / startElementAspect_);
			} else {
				newElementWidth = (newElementHeight * startElementAspect_);
			}
			element_.style.width = newElementWidth + "px";
			element_.style.height = newElementHeight + "px";
		} else if (newElementWidth !== 0) {
			element_.style.width = newElementWidth + "px";
		} else if (newElementHeight !== 0) {
			element_.style.height = newElementHeight + "px";
		} else {
			element_.style.left = (startElementLeft_ + touchDX) + "px";
			element_.style.top = (startElementTop_ + touchDY) + "px";

			var curTime = window.performance.now();
			if ((curTime - lastHitTestTime_) >= G_VAL_MIN_HIT_TEST_INTERVAL_MS) {
				var elementClientRect = element_.getBoundingClientRect();
				/** @type {Range} */
				var range = document.caretRangeFromPoint(
					elementClientRect.left + startElementWidth_ * 0.5,
					elementClientRect.top + startElementHeight_ * 0.5);
				if (range && utils.isElementEditable(range.startContainer)) {
					utils.setSelectionFromRange(range);
				} else {
					utils.clearSelection();
				}
				lastHitTestTime_ = curTime;
			}
		}

		if (dragX_ === 1) {
			element_.style.left = (startElementLeft_ + startElementWidth_ - newElementWidth) + "px";
		}
		if (dragY_ === 1) {
			element_.style.top = (startElementTop_ + startElementHeight_ - newElementHeight) + "px";
		}
	}

	document.addEventListener("touchstart", function (event) {
		if (activeTouchId_ >= 0) {
			return;
		}

		var touch = event.changedTouches[0];
		activeTouchId_ = touch.identifier;
		touchStartTime_ = window.performance.now();

		if (targetElement_ && startDrag(touch)) {
			event.preventDefault();
			utils.sendUserEvent("image-drag-start");
			return;
		}

		var curElement = document.elementFromPoint(touch.clientX, touch.clientY);
		if (curElement && (curElement.tagName === "IMG") && utils.isElementEditable(curElement)) {
			candidateElement_ = curElement;
		} else {
			candidateElement_ = null;
		}
	});

	document.addEventListener("touchend", function (event) {
		if (!utils.findChangedTouchById(event, activeTouchId_)) {
			return;
		}
		activeTouchId_ = -1;

		if (!isDragging_) {
			if (candidateElement_ && ((window.performance.now() -
				touchStartTime_) < G_VAL_LONG_TAP_DELAY_MS)) {

				activate();
				utils.clearSelection();
				event.preventDefault();
			}
			return;
		}
		isDragging_ = false;

		event.preventDefault();
		utils.sendUserEvent("image-drag-end");

		if ((dragX_ === 2) && (dragY_ === 2)) {
			var sel = window.getSelection();
			var range = (sel.rangeCount && sel.getRangeAt(0));
			if (range) {
				range.insertNode(targetElement_);
				utils.clearSelection();
			}
			deactivate();
		} else {
			targetElement_.setAttribute("width", parseFloat(element_.style.width));
			targetElement_.setAttribute("height", parseFloat(element_.style.height));
			updatePosition();
		}
	});

	document.addEventListener("touchmove", function (event) {
		var touch = utils.findChangedTouchById(event, activeTouchId_);
		if (!touch) {
			return;
		}

		if (!isDragging_) {
			candidateElement_ = null;
			return;
		}

		event.preventDefault();

		doDrag(touch);
	});

	document.addEventListener("focusin", function () {
		if (!isDragging_) {
			deactivate();
		}
	});
}

function animationStepCb(timestamp) {
	document.body.scrollTop = g_scrollTop;
	g_frameRequestId = 0;
}

/* Used by c-code */

function SetScrollTop(pos) {
	pos = Math.round(pos / G_VAL_PIXEL_RATIO);
	if (pos !== g_scrollTop) {
		g_scrollTop = pos;
		if (!g_frameRequestId) {
			g_frameRequestId = requestAnimationFrame(animationStepCb);
		}
	}

	g_imageLayer.cancel();
}

function InitializeEmailComposer(elmScaleSize, resDirUri) {
	utils.log(arguments.callee.name + "()");

	if (elmScaleSize) {
		G_VAL_CSS_SCALE_RATIO *= parseFloat(elmScaleSize);
		utils.log(arguments.callee.name + "() elmScaleSize: " + elmScaleSize +
			"; as float: " + parseFloat(elmScaleSize));
	}

	G_VAL_IMAGE_OUTLINE_WIDTH *= G_VAL_CSS_SCALE_RATIO;
	G_VAL_IMAGE_HANDLE_SIZE *= G_VAL_CSS_SCALE_RATIO;
	G_VAL_IMAGE_HANDLE_TOUCH_RADIUS *= G_VAL_CSS_SCALE_RATIO;
	G_VAL_IMAGE_MIN_SIZE *= G_VAL_CSS_SCALE_RATIO;

	if (resDirUri) {
		G_VAL_IMAGE_HANDLE_IMG_PATH = resDirUri + "/" + G_VAL_IMAGE_HANDLE_IMG_PATH;
		utils.log(arguments.callee.name + "() resDirUri: " + resDirUri);
	}

	var originalMessageBar = document.getElementById(G_NAME_ORG_MESSAGE_BAR);
	if (originalMessageBar) {
		originalMessageBar.addEventListener('touchstart', function () { OnTouchStart_OriginalMessageBar(event); }, false);
		originalMessageBar.addEventListener('touchmove', function () { OnTouchMove_OriginalMessageBar(event); }, false);
		originalMessageBar.addEventListener('touchend', function () { OnTouchEnd_OriginalMessageBar(event); }, false);
		var originalMessageBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);
		if (originalMessageBarCheckbox) {
			originalMessageBarCheckbox.addEventListener('keydown', function () { OnkeyDown_Checkbox(this, event); }, false);
		}

		var newMessage = document.getElementById(G_NAME_NEW_MESSAGE);
		if (newMessage) {
			newMessage.addEventListener('focus', function () { OnFocus_NewMessageDIV(event); }, false);
			newMessage.addEventListener('paste', function () { OnPaste(this, event); }, false);
			newMessage.addEventListener('keydown', function () { OnKeyDown(this, event); }, false);
		}

		var originalMessage = document.getElementById(G_NAME_ORG_MESSAGE);
		if (originalMessage) {
			originalMessage.addEventListener('focus', function () { OnFocus_OrgMessageDIV(event); }, false);
			originalMessage.addEventListener('paste', function () { OnPaste(this, event); }, false);
			originalMessage.addEventListener('keydown', function () { OnKeyDown(this, event); }, false);
		}
	} else {
		document.body.addEventListener('paste', function () { OnPaste(this, event); }, false);
		document.body.addEventListener('keydown', function () { OnKeyDown(this, event); }, false);
	}

	g_imageLayer = new ImageLayer();

	document.body.addEventListener('DOMNodeInserted', function () { OnNodeInserted(this, event); }, false);

	document.addEventListener('selectionchange', function () { OnSelectionChanged(); }, false);

	window.addEventListener("scroll", function () {
		if (!g_frameRequestId) {
			var scrollTop = document.body.scrollTop;
			if (scrollTop !== g_scrollTop) {
				utils.log("Prevent scroll: " + scrollTop);
				g_frameRequestId = requestAnimationFrame(animationStepCb);
			}
		}
	});
}

function OnSelectionChanged() {
	utils.log(arguments.callee.name + "()");
	var res = GetCurrentFontStyleProperties(false);
	if (res != null) {
		utils.sendUserEvent("text-style-changed:" + res);
	}
	NotifyCaretPosChange(false);
}

function GetCurFontParamsString() {
	var temp = g_curBgColor;
	if (g_curBgColor == "rgba(0, 0, 0, 0)") {
		temp = G_VAL_DEFAULT_BG_COLOR;
	}

	return "font_size=" + g_curFontSize + "&bold=" + g_curBold +
		"&underline=" + g_curUnderline + "&italic=" + g_curItalic +
		"&font_color=" + g_curFontColor + "&bg_color=" + temp +
		"&ordered_list=" + g_curOrderedList + "&unordered_list=" + g_curUnorderedList;
}

function HexToRgb(hex) {
	var bigint = parseInt(hex, 16);
	var r = (bigint >> 16) & 255;
	var g = (bigint >> 8) & 255;
	var b = bigint & 255;

	return r + "," + g + "," + b;
}

function ExecCommand(type, value) {
	var selectionState = GetSelectionState();
	switch (type) {
		case "bold":
			document.execCommand('bold', false, null);
			if (!selectionState) {
				g_curBold = (g_curBold == 1) ? 0 : 1;
			}
			break;
		case "italic":
			document.execCommand('italic', false, null);
			if (!selectionState) {
				g_curItalic = (g_curItalic == 1) ? 0 : 1;
			}
			break;
		case "underline":
			document.execCommand('underline', false, null);
			if (!selectionState) {
				g_curUnderline = (g_curUnderline == 1) ? 0 : 1;
			}
			break;
		case "fontColor":
			document.execCommand('ForeColor', false, value);
			if (!selectionState) {
				g_curFontColor = "rgb(" + HexToRgb(value) + ")";
			}
			break;
		case "backgroudColor":
			document.execCommand('BackColor', false, value);
			if (!selectionState) {
				g_curBgColor = "rgb(" + HexToRgb(value) + ")";
			}
			break;
		case "fontSize":
			document.execCommand('FontSize', false, value);
			if (!selectionState) {
				g_curFontSize = FontSize[value] + "px";
			}
			break;
		case "orderedList":
			document.execCommand('insertOrderedList', false, null);
			if (!selectionState) {
				g_curOrderedList = (g_curOrderedList == 1) ? 0 : 1;
				if (g_curOrderedList == 1 && g_curUnorderedList == 1) {
					g_curUnorderedList = 0;
				}
			}
			break;
		case "unorderedList":
			document.execCommand('insertUnorderedList', false, null);
			if (!selectionState) {
				g_curUnorderedList = (g_curUnorderedList == 1) ? 0 : 1;
				if (g_curUnorderedList == 1 && g_curOrderedList == 1) {
					g_curOrderedList = 0;
				}
			}
			break;
		default:
			return;
	}

	if (selectionState) {
		utils.log(arguments.callee.name + "()");
		var font_params = GetCurrentFontStyleProperties(false);
		if (font_params != null) {
			utils.sendUserEvent("text-style-changed:" + font_params);
		}
	} else {
		utils.sendUserEvent("text-style-changed:" + GetCurFontParamsString());
	}
}

function GetCurrentFontStyle() {
	var res = GetCurrentFontStyleProperties(true);
	if (res != null) {
		utils.sendUserEvent("text-style-changed:" + res);
		return 1;
	} else {
		return 0;
	}
}

function IsCheckboxChecked() {
	utils.log(arguments.callee.name + "()");

	var orgMsgBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);

	if (orgMsgBarCheckbox && (orgMsgBarCheckbox.className == G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED)) {
		return 'true';
	} else {
		return 'false';
	}
}

function HasOriginalMessageBar() {
	utils.log(arguments.callee.name + "()");

	if (document.getElementById(G_NAME_ORG_MESSAGE_BAR) != null) {
		return 'true';
	} else {
		return 'false';
	}
}

function InsertOriginalMessageText(r, g, b, a, text) {
	utils.log(arguments.callee.name + "()");

	var newSpan = document.createElement("span");
	newSpan.setAttribute("id", G_NAME_ORG_MESSAGE_BAR_TEXT);
	newSpan.style.cssText = G_VAL_ORG_MESSAGE_BAR_TEXT_ALIGN + " font-size:" + G_VAL_ORG_MESSAGE_BAR_TEXT_SIZE + "px; color: rgb(" + r + "," + g + "," + b + "); opacity=" + a / 255.0 + ";"
	newSpan.innerHTML = text;

	var orgMessageBarDivRow = document.getElementById(G_NAME_ORG_MESSAGE_BAR_ROW);
	orgMessageBarDivRow.insertBefore(newSpan, orgMessageBarDivRow.firstChild);
}

function UpdateOriginalMessageText(text) {
	utils.log(arguments.callee.name + "()");
	document.getElementById(G_NAME_ORG_MESSAGE_BAR_TEXT).innerHTML = text;
}

function InsertOriginalMailInfo(mail_info) {
	utils.log(arguments.callee.name + "()");

	var orgMessageDiv = document.getElementById(G_NAME_ORG_MESSAGE);
	var mailInfoDiv = document.createElement("div");
	mailInfoDiv.innerHTML = mail_info;
	orgMessageDiv.insertBefore(mailInfoDiv, orgMessageDiv.firstChild);
}

function InsertSignature(signature) {
	utils.log(arguments.callee.name + "()");
	var signatureDiv = document.createElement("div");
	signatureDiv.innerHTML = signature;
	document.body.appendChild(signatureDiv);
}

function InsertSignatureTo(div_id, signature) {
	utils.log(arguments.callee.name + "()");
	var signatureDiv = document.createElement("div");
	signatureDiv.innerHTML = signature;
	document.getElementById(div_id).appendChild(signatureDiv);
}

function UpdateBGColorTo(div_id, r, g, b, a) {
	utils.log(arguments.callee.name + "()");
	var targetDiv = document.getElementById(div_id);
	targetDiv.style.backgroundColor = "rgb(" + r + "," + g + "," + b + ")";
	targetDiv.style.opacity = a / 255.0;
}

function InsertImage(fileUri) {
	utils.log(arguments.callee.name + "()");

	var selection = window.getSelection();
	if (selection.rangeCount === 0) {
		return;
	}
	var range = selection.getRangeAt(0);

	var img = document.createElement("img")
	img.onload = function () {
		var w = img.width;
		var h = img.height;
		var maxWidth = Math.min(screen.width, screen.height) * G_VAL_IMAGE_INITIZL_SIZE;
		if (w > maxWidth) {
			h *= maxWidth / w;
			w = maxWidth;
		}

		img.setAttribute("width", w);
		img.setAttribute("height", h);
		img.style.cssText = G_VAL_IMAGE_STYLE;
	}
	img.src = fileUri;
	img.style.display = "none";

	range.deleteContents();
	range.insertNode(img);
	range.collapse(false);
	selection.removeAllRanges();
	selection.addRange(range);
}

function GetImageSourcesFromElement(element) {
	utils.log(arguments.callee.name + "()");
	var images = element.getElementsByTagName("img");
	var imagesMax = images.length - 1;
	var curImg, curImgSrc, srcs = "", idCounter = 0, re = /;/g;
	for (var i = 0; i <= imagesMax; i++) {
		curImg = images[i];
		curImgSrc = curImg.src;
		if (curImgSrc.startsWith("file://")) {
			curImg.dataset["id"] = ++idCounter;
			srcs += curImgSrc.replace(re, "%3B") + ((i < imagesMax) ? ";" : "");
		}
	}
	return srcs;
}

function GetImageSources() {
	return GetImageSourcesFromElement(document.body);
}

function GetImageSourcesFrom(div_id) {
	return GetImageSourcesFromElement(document.getElementById(div_id));
}

function GetComposedHtmlContents(inlineImageSrcs) {
	utils.log(arguments.callee.name + "()");

	var srcNewMsg = document.getElementById(G_NAME_NEW_MESSAGE);
	var srcOrgMsg = srcNewMsg && document.getElementById(G_NAME_ORG_MESSAGE);

	var dstContent = document.createElement("html");

	/** @type {HTMLElement} */
	var dstBody;

	if (!srcOrgMsg || (IsCheckboxChecked() != "true")) {
		if (!srcNewMsg) {
			dstBody = document.body.cloneNode(true);
			// TODO Temp code. Remove when replace body with DIV.
			var imgLayers = dstBody.getElementsByClassName("email-composer-image-layer");
			if (imgLayers.length > 0) {
				imgLayers[0].parentElement.removeChild(imgLayers[0]);
			}
		} else {
			dstBody = srcNewMsg.cloneNode(true);
			dstBody.removeAttribute("id");
			dstBody.removeAttribute("class");
		}
		dstBody.removeAttribute("contenteditable");
	} else {
		dstBody = document.createElement("body");

		var tmp = srcNewMsg.cloneNode(true);
		tmp.removeAttribute("id");
		tmp.removeAttribute("class");
		tmp.removeAttribute("contenteditable");
		dstBody.appendChild(tmp);

		tmp = document.createElement("br");
		dstBody.appendChild(tmp);
		dstBody.appendChild(tmp.cloneNode(false));

		tmp = srcOrgMsg.cloneNode(true);
		tmp.removeAttribute("id");
		tmp.removeAttribute("class");
		tmp.removeAttribute("contenteditable");
		dstBody.appendChild(tmp);
	}

	dstContent.appendChild(dstBody);

	if (typeof inlineImageSrcs === "string") {
		try {
			inlineImageSrcs = JSON.parse(inlineImageSrcs);
			if (typeof inlineImageSrcs !== "object") {
				inlineImageSrcs = null;
			}
		} catch (error) {
			utils.log(arguments.callee.name + "() " + error);
			inlineImageSrcs = null;
		}
	} else {
		inlineImageSrcs = null;
	}

	var images = dstBody.getElementsByTagName("img");
	var imagesLength = images.length;
	var i = 0;
	while (i < imagesLength) {
		var curImg = images[i];
		if (curImg.src.startsWith("file://")) {
			if (inlineImageSrcs) {
				/** @type {string} */
				var newImgSrc = inlineImageSrcs[curImg.dataset["id"]];
				if (newImgSrc) {
					curImg.src = newImgSrc;
				} else if (G_VAL_REMOVE_INVALID_IMAGES) {
					curImg.parentNode.removeChild(curImg);
					--imagesLength;
					continue;
				}
			}
			curImg.removeAttribute("data-id");
		}
		++i;
	}

	return dstContent.outerHTML;
}

function GetCaretPosition(force) {
	var rect;
	var isCollapsed = 1;

	var lastRange = g_lastRange;
	g_lastRange = null;

	var sel = window.getSelection();

	if (sel.rangeCount > 0) {
		isCollapsed = sel.isCollapsed ? 1 : 0;
		var range = sel.getRangeAt(0);
		var rects = range.getClientRects();
		if (rects.length > 0) {
			if (lastRange) {
				if (range.compareBoundaryPoints(Range.START_TO_START, lastRange) != 0) {
					g_lastRangeIsForward = false;
				} else if (range.compareBoundaryPoints(Range.END_TO_END, lastRange) != 0) {
					g_lastRangeIsForward = true;
				} else if (!force && !isCollapsed) {
					return null;
				}
			}
			rect = rects[g_lastRangeIsForward ? (rects.length - 1) : 0];
			g_lastRange = range;
		} else {
			var span = document.createElement("span");
			span.style.cssFloat = "left";
			span.appendChild(document.createTextNode("\u200b"));
			range.insertNode(span);
			var rects = range.getClientRects();
			if (rects.length > 0) {
				rect = rects[0];
			}
			var spanParent = span.parentNode;
			spanParent.removeChild(span);
			spanParent.normalize();
		}
	} else {
		var node = document.activeElement;
		if (node === document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX)) {
			var rects = node.getClientRects();
			if (rects.length > 0) {
				rect = rects[0];
			}
		}
	}

	if (!rect) {
		return null;
	}

	var scrollY = window.scrollY;

	return new CaretPos(
		(rect.top + scrollY) * G_VAL_PIXEL_RATIO,
		(rect.bottom + scrollY) * G_VAL_PIXEL_RATIO,
		isCollapsed);;
}

function RestoreCurrentSelection() {
	utils.log(arguments.callee.name + "()");

	if (g_selectionRange) {
		utils.setSelectionFromRange(g_selectionRange);
	}
}

function BackupCurrentSelection() {
	utils.log(arguments.callee.name + "()");

	var selection = window.getSelection();
	if (selection.rangeCount > 0) {
		g_selectionRange = selection.getRangeAt(0);
	}
}

function GetSelectionState() {
	BackupCurrentSelection();
	if (g_selectionRange != null && g_selectionRange.toString() != "") {
		return true;
	}
	return false;
}

function GetComputedStyle(el) {
	return window.getComputedStyle(el, null);
}

function GetComputedStyleProperty(el, propName) {
	return window.getComputedStyle(el, null)[propName];
}

function GetCurrentFontStyleProperties(force) {
	var nodeArray = getSelectedNodes();

	if (nodeArray.length == 0) {
		return null;
	}

	var containerEl = null;
	var newFontSize = "", newBold = 1, newUnderline = 1, newItalic = 1, newOrderedList = 1, newUnorderedList = 1;
	var newFontColor = "", newBgColor = "";

	if (nodeArray.length == 1 && nodeArray[0].nodeType != G_VAL_TEXT_NODE_TYPE) {

		newBold = document.queryCommandState('bold') ? 1 : 0;
		newUnderline = document.queryCommandState('underline') ? 1 : 0;
		newItalic = document.queryCommandState('italic') ? 1 : 0;
		newOrderedList = document.queryCommandState('insertOrderedList') ? 1 : 0;
		newUnorderedList = document.queryCommandState('insertUnorderedList') ? 1 : 0;
		newFontColor = document.queryCommandValue('forecolor');
		newBgColor = document.queryCommandValue('backcolor');

		var tempSize = document.queryCommandValue('FontSize');
		if (tempSize == 0) {
			newFontSize = G_VAL_DEFAULT_FONT_SIZE;
		} else {
			newFontSize = FontSize[tempSize] + "px";
		}
	} else {
		for (var i in nodeArray) {
			containerEl = nodeArray[i];
			if (containerEl.nodeType == G_VAL_TEXT_NODE_TYPE) {
				containerEl = containerEl.parentNode;
			}

			var iFontSize = "", iBold = 0, iUnderline = 0, iItalic = 0, iOrderedList = 0, iUnorderedList = 0;
			var iFontColor = "", iBgColor = "";

			var computedStyle = GetComputedStyle(containerEl);
			iFontSize = computedStyle["fontSize"];
			iBold = (computedStyle["font-weight"] == "bold") ? 1 : 0;
			iItalic = (computedStyle["fontStyle"] == "italic") ? 1 : 0;
			iFontColor = computedStyle["color"];
			iBgColor = computedStyle["backgroundColor"];

			while (containerEl.nodeName.toLowerCase() != "body") {
				switch (containerEl.nodeName.toLowerCase()) {
					case "em":
						iItalic = 1;
						break;
					case "strong":
						iBold = 1;
						break;
					case "ol":
						if (iOrderedList == 0 && iUnorderedList == 0) {
							iOrderedList = 1;
							iUnorderedList = 0;
						}
						break;
					case "ul":
						if (iOrderedList == 0 && iUnorderedList == 0) {
							iOrderedList = 0;
							iUnorderedList = 1;
						}
						break;
					default:
						break;
				}

				if (iBgColor == "rgba(0, 0, 0, 0)") {
					iBgColor = GetComputedStyleProperty(containerEl, "backgroundColor");
				}
				if (iUnderline == 0) {
					iUnderline = (GetComputedStyleProperty(containerEl, "text-decoration") == "underline") ? 1 : 0;
				}
				containerEl = containerEl.parentNode;
			}

			newBold = newBold & iBold;
			newUnderline = newUnderline & iUnderline;
			newItalic = newItalic & iItalic;
			newOrderedList = newOrderedList & iOrderedList;
			newUnorderedList = newUnorderedList & iUnorderedList;

			if (newFontSize == "") {
				newFontSize = iFontSize;
			} else if (newFontSize != iFontSize) {
				newFontSize = G_VAL_DEFAULT_FONT_SIZE;
			}

			if (newFontColor == "") {
				newFontColor = iFontColor;
			} else if (newFontColor != iFontColor) {
				newFontColor = G_VAL_DEFAULT_FONT_COLOR;
			}

			if (newBgColor == "") {
				newBgColor = iBgColor;
			} else if (newBgColor != iBgColor) {
				newBgColor = G_VAL_DEFAULT_BG_COLOR;
			}
		}
	}

	if ((force == false &&
		g_curFontSize == newFontSize && g_curBold == newBold &&
		g_curUnderline == newUnderline && g_curItalic == newItalic &&
		g_curOrderedList == newOrderedList && g_curUnorderedList == newUnorderedList &&
		g_curFontColor == newFontColor && g_curBgColor == newBgColor) || newFontSize == 0) {
		return null;
	} else {
		g_curFontSize = newFontSize;
		g_curBold = newBold;
		g_curUnderline = newUnderline;
		g_curItalic = newItalic;
		g_curOrderedList = newOrderedList;
		g_curUnorderedList = newUnorderedList;
		g_curFontColor = newFontColor;
		g_curBgColor = newBgColor;
		return GetCurFontParamsString();
	}
}

function nextNode(node) {
	if (node.hasChildNodes()) {
		return node.firstChild;
	} else {
		while (node && !node.nextSibling) {
			node = node.parentNode;
		}
		if (!node) {
			return null;
		}
		return node.nextSibling;
	}
}

function getRangeSelectedNodes(range) {
	var node = range.startContainer;
	var endNode = range.endContainer;
	var rangeNodes = [];

	while (node) {
		if (node.nodeType == G_VAL_TEXT_NODE_TYPE) {
			rangeNodes.push(node);
		}
		if (node == endNode) {
			break;
		}
		node = nextNode(node);
	}

	if (rangeNodes.length == 0) {
		rangeNodes.push(range.commonAncestorContainer);
	}

	return rangeNodes;
}

function getSelectedNodes() {
	if (window.getSelection) {
		var sel = window.getSelection();
		if (sel.rangeCount > 0) {
			return getRangeSelectedNodes(sel.getRangeAt(0));
		}
	}
	return [];
}
