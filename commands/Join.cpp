



#include "include/Command.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"
#include "include/Channel.hpp"

class JoinCommand : public Command
{
    public:
        void execute(Server &server, Client &client, const std::vector<std::string> &params)
        {
            // 1. التحقق من وجود المعاملات (هل كتب اسم الغرفة؟)
            if (params.empty())
            {
                client.sendMessage(":server 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
                return;
            }

            std::string channelName = params;
            
            // 2. البحث عن الغرفة في الخادم
            Channel *channel = server.findChannel(channelName);
            bool isNewChannel = false;

            // 3. إذا لم تكن موجودة، قم بإنشائها
            if (channel == NULL)
            {
                channel = server.createChannel(channelName);
                isNewChannel = true;
            }

            // (هنا مستقبلاً سنضع شروط الـ Modes مثل كلمة السر والدعوة)

            // 4. إضافة العميل للغرفة
            channel->addMember(&client);

            // 5. إذا كان هو من أنشأ الغرفة، نجعله المشرف (Operator)
            if (isNewChannel)
            {
                channel->addOperator(&client);
            }

            // 6. تجهيز رسالة الانضمام الرسمية الخاصة ببروتوكول IRC
            // البادئة يجب أن تكون: :nickname!username@localhost
            std::string userPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
            std::string joinMsg = userPrefix + " JOIN :" + channelName + "\r\n";

            // 7. بث الرسالة لجميع من في الغرفة (أخبرهم أن هذا الشخص دخل)
            channel->broadcastMessage(joinMsg, &client); 
            
            // 8. إرسال الرسالة للشخص نفسه أيضاً (لكي يعرف برنامجه أنه دخل بنجاح)
            client.sendMessage(joinMsg);

            // 9. إرسال موضوع الغرفة (إن وُجد)
            if (!channel->getTopic().empty())
            {
                client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
            }

            // ملاحظة للمستقبل: هنا يطلب الـ IRC إرسال قائمة بأسماء الأعضاء (الأرقام 353 و 366).
            // سنقوم بإضافتها لاحقاً لكي لا نزيد الكود تعقيداً الآن.
        }
};

// الدالة التي يستخدمها الخادم (Server) لإنشاء هذا الأمر
Command* createJoinCommand() { return new JoinCommand(); }