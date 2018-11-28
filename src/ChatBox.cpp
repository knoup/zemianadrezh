#include "ChatBox.h"

#include "FontManager.h"

#include "Keybinds.h"

//Here are the variables we'll use to initialise the text entry box
//in the correct position
//Note that the height (0.25 by default) specifies that of both the
//ChatBox and m_textEntry combined.
//////////////////////////////////////////////////////////////////////////////////////////////
const sf::FloatRect VIEWPORT{
	0, 0.75, 0.3, 0.25
};
constexpr float         SECONDS_UNTIL_MESSAGES_FADE {5.0f};
constexpr unsigned int  CHARACTER_SIZE       {20};

const  float            LINESPACING = FontManager::get_instance().getLineSpacing(FontManager::Type::ANDY, CHARACTER_SIZE);
const float             Y_BUFFERSPACE{1.3f * LINESPACING};

//The value of 1.3 above is because the origin of the text's position is top
//left, in order to get a little aesthetic extra buffer space below the
//last message
//////////////////////////////////////////////////////////////////////////////////////////////

ChatBox::ChatBox(sf::RenderWindow& _window, const std::string& _name)
	:m_name(_name),
	 m_view(),
	 m_shadedRectangleView(),
	 m_shadedRectangle(),
	 m_messages(),
	 m_textEntry(),
	 m_clock() {

	onResize(_window.getSize());

	/*
	appendMessage("Impending doom approaches... Also this is a test to see if the splitter function works properly");
	appendMessage("message1", "Test");
	appendMessage("message2", "Test");
	appendMessage("message3", "Test");
	appendMessage("message4", "Test");
	appendMessage("message5", "Test");

	appendMessage("message6", "Test");
	appendMessage("message7", "Test");
	appendMessage("message8", "Test");
	appendMessage("message9", "Test");
	*/

	snapToBottom();

}

void ChatBox::appendMessage(const std::string _message,
							const std::string _sender) {
	sf::MulticolourText newText;
	newText.setFont(FontManager::get_instance().getFont(FontManager::Type::ANDY));
	newText.setCharacterSize(CHARACTER_SIZE);

	std::string finalStr{_message};
	if(_sender != "") {
		finalStr = ("<" + _sender + "> " + finalStr);
	}

	newText.setString(finalStr);
	newText.setFillColor(sf::Color::Red, 1, _sender.length() + 1);

	Message newMessage{newText};

	if(messageTooWide(newMessage)) {
		adjustMessage(newMessage);
	}

	bool snap{false};
	if(viewAtLowest()){
        snap = true;
	}

	m_messages.push_back(newMessage);
	positionMessage(m_messages.size() - 1);
	m_clock.restart();

	if(snap){
        snapToBottom();
	}
}

void ChatBox::getInput(sf::Event& _event) {
	//In the case of the enter key, we're going to poll an event.
	//The reason for this is that we only want to toggle this once,
	//and not have it rapidly be called every frame.
	m_textEntry.getInput(_event);

	switch(_event.type) {
	case sf::Event::KeyPressed: {
			if(_event.key.code == Key::CHAT_SEND) {
				//snapToBottom();
				m_clock.restart();
			}

			else if(_event.key.code == Key::CHAT_UP) {
				m_clock.restart();
				scrollUp();
			}

			else if(_event.key.code == Key::CHAT_DOWN) {
				m_clock.restart();
				scrollDown();
			}

			else if(_event.key.code == Key::CHAT_TOP) {
				m_clock.restart();
				snapToTop();
			}

			else if(_event.key.code == Key::CHAT_BOTTOM) {
				m_clock.restart();
				snapToBottom();
			}

			break;
		}

	case sf::Event::Resized: {
			sf::Vector2u newSize{_event.size.width, _event.size.height};
			onResize(newSize);
			break;
		}
	default:
		break;
	}

}

void ChatBox::update() {
	m_textEntry.update();
	if(m_textEntry.inputComplete()) {
		//appendMessage(m_textEntry.getLastString(), m_name);
		m_lastMessage = std::make_pair( m_textEntry.getLastString(),
										m_name);
	}
	updateShadedRectangleTransparency();
	updateMessageTransparency();
}

void ChatBox::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	sf::View previousView = target.getView();

	target.setView(m_shadedRectangleView);
	target.draw(m_shadedRectangle, states);

	target.setView(m_view);
	if(!messagesTransparent()) {
		for(auto& message : m_messages) {
			target.draw(message.text, states);
		}
	}

	target.draw(m_textEntry, states);

	target.setView(previousView);
}


bool ChatBox::completedMessage(std::pair<std::string, std::string>* _ptr) {
	if(m_lastMessage.first == "" && m_lastMessage.second == "") {
		return false;
	}

	*_ptr = m_lastMessage;
	m_lastMessage.first = "";
	m_lastMessage.second = "";

	return true;
}

const bool ChatBox::messageTooWide(Message& _message) const {
	return _message.text.getGlobalBounds().width >= m_view.getSize().x * 0.9;
}

const bool ChatBox::messageTooNarrow(Message& _message) const {
	bool containsNewline{false};
	auto i = _message.text.getString().find("\n");
	if(i != std::string::npos) {
		containsNewline = true;
	}

	return (_message.text.getGlobalBounds().width < m_view.getSize().x * 0.9)
		   &&
		   containsNewline;
}

void removeNewlines(std::string& _str) {
	std::string newline{"\n"};
	auto i = _str.find(newline);
	while(i != std::string::npos) {
		_str.erase(i, newline.length());
		i = _str.find(newline, i);
	}
}

void ChatBox::adjustMessage(Message& _message) {
	if(!messageTooWide(_message) && !messageTooNarrow(_message)){
		return;
	}

	//Before we recalculate everything, we'll remove
	//all newline characters currently present and
	//set the message's numberOfLines to 1
	std::string textStr = _message.text.getString();
	removeNewlines(textStr);
	_message.text.setString(textStr);
	_message.numberOfLines = 1;

	float charSize = _message.text.getGlobalBounds().width / textStr.size();
	auto widthLimit = m_view.getSize().x * 0.9;

	float currentWidth{0};

	for(size_t i{0}; i < textStr.length(); i++) {
		currentWidth += charSize;

		//Insert a newline at the position, if it's
		//too wide
		if(currentWidth >= widthLimit) {
			textStr.insert(i, 1, '\n');
			_message.numberOfLines++;
			currentWidth = 0;
		}

		//If we have a space as the first character
		//on a new line, we'll get rid of it
		/*
		else if(currentWidth == charSize) {
			if(isspace(textStr.at(i))) {
				textStr.erase(i, 1);
			}
		}

		... or not. A bit funky with MultiColorText::setString()
		*/
	}

	_message.text.setString(textStr);
}

//This function sets the position of a new message, at the
//very bottom of the box
void ChatBox::positionMessage(int _index) {
	if(!m_messages.empty() && _index > 0) {
		Message& message = m_messages[_index];
		sf::Vector2f newPosition{m_messages[_index-1].text.getPosition()};
		unsigned int lastMessageLines{m_messages[_index-1].numberOfLines};
		newPosition.y += lastMessageLines * (LINESPACING);
		message.text.setPosition(newPosition);
	}
}


void ChatBox::setTransparency(int _a) {
	for(auto& message : m_messages) {
		message.text.setTransparency(_a);
	}
}


bool ChatBox::messagesTransparent() const {
	//Since all messages will have the same transparency,
	//it's enough for us to get the alpha value of the first
	//element
	if(!m_messages.empty()) {
		return m_messages.front().text.getFillColor(0).a == 0;
	}
}

void ChatBox::updateShadedRectangleTransparency() {
	static int rectangleAlphaValue{0};

	if(m_textEntry.enteringText()) {
		rectangleAlphaValue = 50;
	}
	else {
		if(rectangleAlphaValue > 0) {
			rectangleAlphaValue -= 1;
		}
	}

	m_shadedRectangle.setFillColor(sf::Color(0,0,0,rectangleAlphaValue));
}

void ChatBox::updateMessageTransparency() {
	static int textAlphaValue{255};

	if(m_textEntry.enteringText()) {
		textAlphaValue = 255;
	}
	else {
		if(m_clock.getElapsedTime().asSeconds() > SECONDS_UNTIL_MESSAGES_FADE) {
			if(textAlphaValue > 0) {
				textAlphaValue -= 1;
			}
		}
		//The clock might have been reset due to the user scrolling
		//up or down, despite m_enteringText being false.
		//In this case, we still want the text to be visible.
		else {
			textAlphaValue = 255;
		}
	}

	setTransparency(textAlphaValue);
}


void ChatBox::onResize(sf::Vector2u _newSize) {
	sf::FloatRect textEntryViewport{
		0,
		1 - m_textEntry.getHeightAsRatio(_newSize),
		VIEWPORT.width,
		0	//this can be removed as it's unused now
	};

	m_textEntry.initialise(_newSize, textEntryViewport);

    float newViewportHeight{VIEWPORT.height - (m_textEntry.getHeightAsRatio(_newSize))};

    //I noticed that sometimes, maybe due to float precision reasons, the text entry box
	//and the shaded rectangle overlap. Because I can't reduce the size of the shaded
	//rectangle pixel by pixel easily, I tried decrementing it by 0.0001% until they no
	//longer overlap and there is no visible discrepancy. My do...while loop tests showed
	//that it just needed to be called once to fix this weird visual quirk.
	newViewportHeight -= 0.0001;

	sf::FloatRect viewRect({0,
							0,
							_newSize.x * VIEWPORT.width,
							_newSize.y * newViewportHeight
						   });




	m_view.reset(viewRect);
	m_view.setViewport({VIEWPORT.left,
                        VIEWPORT.top,
                        VIEWPORT.width,
                        newViewportHeight});

	m_shadedRectangleView.reset(viewRect);
	m_shadedRectangleView.setViewport(m_view.getViewport());

	m_shadedRectangle.setSize(m_shadedRectangleView.getSize());

	for(int i{0}; i < m_messages.size(); i++) {
		Message& message = m_messages[i];
		adjustMessage(message);
		positionMessage(i);
	}

	snapToBottom();
}

//This function checks if the last message is "outside" (below) the view.
//If so, it adjusts the view's center so that the very first message is
//on top.
void ChatBox::snapToTop() {
	if(!m_messages.empty()) {
		Message& latestMessage = m_messages.back();

		float lastLineYPosition{latestMessage.text.getPosition().y
								+
								(latestMessage.numberOfLines - 1) * LINESPACING};

		float boundary{m_view.getSize().y - Y_BUFFERSPACE};

		if(lastLineYPosition < boundary) {
			return;
		}

		Message& earliestMessage = m_messages.front();
		sf::Vector2f newCenter {m_view.getCenter().x, earliestMessage.text.getPosition().y};

		newCenter.y += (m_view.getSize().y / 2);

		m_view.setCenter(newCenter);
	}
}

//This function checks if the last message is "outside" (below) the view.
//If so, it adjusts the view's center so that it is visible.
void ChatBox::snapToBottom() {
	if(!m_messages.empty()) {
		Message& latestMessage = m_messages.back();
		float lastLineYPosition{latestMessage.text.getPosition().y
								+
								(latestMessage.numberOfLines - 1) * LINESPACING};

		float boundary{m_view.getSize().y - Y_BUFFERSPACE};

		if(lastLineYPosition < boundary) {
			return;
		}

		sf::Vector2f newCenter {m_view.getCenter().x, lastLineYPosition};

		newCenter.y -= (m_view.getSize().y / 2) - (Y_BUFFERSPACE);

		m_view.setCenter(newCenter);
	}
}

float ChatBox::getUpperViewBound() const {
	return 0;
}
float ChatBox::getLowerViewBound() const {
    if(m_messages.empty()){
        return 0;
    }
	float lowestPoint{m_messages.back().text.getPosition().y};
	lowestPoint += Y_BUFFERSPACE;
	return lowestPoint;
}

bool ChatBox::viewAtHighest() const {
	if(m_view.getCenter().y - m_view.getSize().y / 2 <= getUpperViewBound()) {
		return true;
	}

	return false;
}

bool ChatBox::viewAtLowest() const {
	if(m_view.getCenter().y + m_view.getSize().y / 2 >= getLowerViewBound()) {
		return true;
	}

	return false;
}

void ChatBox::scrollUp() {
	if(!viewAtHighest()) {
		m_view.setCenter(m_view.getCenter().x, m_view.getCenter().y - (LINESPACING));
	}
	if(viewAtHighest()) {
		snapToTop();
	}
}

void ChatBox::scrollDown() {
	if(!viewAtLowest()) {
		m_view.setCenter(m_view.getCenter().x, m_view.getCenter().y + (LINESPACING));
	}
	if(viewAtLowest()) {
		snapToBottom();
	}
}


