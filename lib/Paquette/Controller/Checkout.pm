package Paquette::Controller::Checkout;

use Moose;
use Data::Dumper;
use Net::UPS;
use Business::PayPal;
BEGIN { extends 'Catalyst::Controller' }
use Paquette::Form::Checkout::Customer;
use Paquette::Form::Checkout::Shipping;
use Paquette::Form::Checkout::Payment;

has 'customer_form' => ( 
    isa => 'Paquette::Form::Checkout::Customer', 
    is => 'rw',
    lazy => 1, 
    default => sub { Paquette::Form::Checkout::Customer->new } 
);
has 'shipping_form' => (
    isa => 'Paquette::Form::Checkout::Shipping',
    is => 'rw',
    lazy => 1,
    default => sub { Paquette::Form::Checkout::Shipping->new }
);
has 'payment_form' => (
    isa => 'Paquette::Form::Checkout::Payment',
    is => 'rw',
    lazy => 1,
    default => sub { Paquette::Form::Checkout::Payment->new }
);

=head1 NAME

Paquette::Controller::Checkout - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut

sub auto : Local {
    my ( $self, $c ) = @_;
    my $cart_size;
    my $categories;

    $cart_size = $c->model('Cart')->count_items_in_cart;
    # Get all my parent categories
    $categories = [$c->model('PaquetteDB::Categories')->search(
        { parent_id => 0 }
    )];

    $c->stash->{cart_size}      = $cart_size;
    $c->stash->{categories}     = $categories;

}

=head2 auto

Check if there is a user and, if not, forward to login page

=cut

sub begin : Private {
    my ( $self, $c ) = @_;

    if ($c->user_exists && $c->user->email eq 'admin' && 
            $c->action->name ne 'noadmin') {
        $c->response->redirect($c->uri_for('/checkout/noadmin'));
    }

}

sub noadmin : Local {
    my ( $self, $c ) = @_;

    $c->stash->{template} = 'checkout/noadmin.tt2';

}

sub authenticate : Local {
    my ($self, $c) = @_;

    if ($c->controller eq $c->controller('Checkout::Login')) {
        return 1;
    }

    # If a user doesn't exist, force login
    if (!$c->user_exists) {
        # Dump a log message to the development server debug output
        $c->log->debug('Checkout::auto User not found, fwd to /checkout/login');
        # Redirect the user to the login page
        $c->response->redirect($c->uri_for('/checkout/login'));
        # Retr 0 to cancel 'post-auto' processing and prevent use of application
        return 0;
    }

    # User found, so return 1 to continue with processing after this 'auto'
    return 1;
}


=head2 index

=cut

sub index :Path :Args(0) {
    my ( $self, $c ) = @_;
    
}

sub customer : Local {
    my ( $self, $c ) = @_;
    my $auth;
    my $cart;
    my $customer;
    my $username;
    my $password;
    my $form;

    # Check if we are logged in
    if ($c->user_exists) {
    # Customer is logged in
        
        # Get our customer object row
        $customer   = $c->model('Customer')->get_customer($c->user->id);
        $cart       
            = $c->model('Cart')->get_cart( { customer_id => $c->user->id } );
        $c->stash->{cart} = $cart if $cart;

    } else {
    # Custmer is not logged it

        # Hold a new row in the database for our record
        $customer = $c->model('PaquetteDB::Customer')->new_result({});

    }
    if (!$c->req->params->{promo_code}) {
        $c->log->debug("not promo");
    } 

    # Set our template and form to use
    $c->stash( 
        cart_items  => $c->model('Cart')->get_items_in_cart,
        template    => 'checkout/customer.tt2',
        form        => $self->customer_form,
    );
    
    # Process our form
    $form =  $self->customer_form->process (
        item            => $customer,
        params          => $c->req->params,
    );

    # If the form is processed then automatically authenticate the user. 
    # Else return the form with errors.

    if ( $form ) { 
    # The form was processed

        # Set our username and password for authentication
        $username    = $c->req->params->{email};
        $password    = $c->req->params->{password};

        # If username and password are set then authenticate user.
        # Else we would give an error_msg
        if ($username && $password) {
        # The username and password are set

            # Attempt to log the user in
            # Using searchargs to authenticate cause non-standard table layout
            # We can also use it to accept username or nickname
            $auth = $c->authenticate(
            {   

                password    => $password,
                'dbix_class' => {
                    searchargs => [
                        {   
                            'email' => $username,
                        }
                    ]
                },
            }
            );

        }

        # If customer was authenticated, then redirect him to his account
        if ($auth || $c->user_exists) {

            # Assign cart_id/session_id to user
            $c->model('Cart')->assign_cart ( {
                customer_id => $c->user->id,
                session_id  => $c->sessionid,
            
            } ) ;

            # if we have have a promotion code, set it in the cart
            if ($c->req->params->{promo_code}) {
                $c->model('Cart')->set_promo_code(
                    $c->req->params->{'promo_code'}
                );
            }

            # Forward to shipping
            $c->res->redirect( $c->uri_for($self->action_for('shipping')) );
        }

    } else { 
    # Username or password were not defined 

        # TODO: display error message via ->flash->{error_msg} ?
        # Mean while we will display a message in debug
        $c->log->debug("not submited");

        return;

    }

}

sub shipping : Local {
    my ( $self, $c ) = @_;
    my $auth;
    my $customer;
    my $cart;
    my $form;
    my $ups;
    my $pkg;
    my $weight;
    my $shipping_from;
    my $shipping_to;
    my $rate;
    my $promo;
    my $promo_type;
    my $promo_amount;
    my $promo_over;
    my $ship_amount;
    my $total_amount;

    # UPS Shipping
    $ups = Net::UPS->new("eltorito", "eltorito", "0C4E421B6422B040");

    $weight = $c->model('Cart')->total_weight_in_cart;
    # prepare package
    $pkg = Net::UPS::Package->new(
        length  => 1, 
        width   => 1, 
        height  => 1, 
        weight  => $weight
    );

    # If shipping has not been defined and the form has been submited,
    # display an error in the form.
    # Else parse the shipping parameter.
    if (!$c->req->params->{shipping} && $c->req->params->{submit}) {
        
        $c->flash->{form_error} = "Please select a shipping method";

        $c->res->redirect( $c->uri_for($self->action_for('shipping')) );

    } else {

        my @shipping_info = split(/:/, $c->req->params->{shipping});

        $c->req->params->{shipping_type} = $shipping_info[0];
        $c->req->params->{shipping_amount} = $shipping_info[1];
    }

    if ($c->user_exists) {
    # Customer is logged in
       
        # Get our customer object row
        $customer = $c->model('Customer')->get_customer($c->user->id)
    } 

    # Get the customer cart by his id
    $cart = $c->model('Cart')->get_cart({
        customer_id => $customer->id,
    });

    $promo = $c->model('Promotion')->get_promotion($cart->promo_code);

    if ($promo) {
        $promo_type     = $promo->type;
        $promo_amount   = $promo->amount;

        if ($promo_type == 'over') {
            $promo_over     = $promo->over;
        }
    }

    $shipping_from = '33133';
    $shipping_to   = $customer->ship_zip_code;

    $rate = $ups->rate($shipping_from, $shipping_to, $pkg );

    my $shipping;
    my $services = $ups->shop_for_rates( 
        $shipping_from, $shipping_to, $pkg,
        { 
            limit_to => [
                #'2ND_DAY_AIR', 
                #'2ND_DAY_AIR_AM', 
                '3_DAY_SELECT', 
                'GROUND',
            ] 
        }
    );

    $total_amount = $c->model('Cart')->sum_items_in_cart;

    while ( my $service = shift @$services ) {
        
        if ( $promo_type eq "shipping" && $service->label eq "GROUND" ) { 

            my $ship_amount = $service->total_charges - 
                ( $service->total_charges * $promo_amount / 100 );
            #$shipping->{$service->label}->{'rate'} = $service->total_charges;
            $shipping->{$service->label}->{'rate'} = $ship_amount;
            $shipping->{$service->label}->{'days'} = $service->guaranteed_days;
        
        } elsif ( $promo_type eq "over" && $service->label eq "GROUND" ) { 

            if ( $total_amount >= $promo->over ) {
            my $ship_amount = $service->total_charges -
                ( $service->total_charges * $promo_amount / 100 );
            #$shipping->{$service->label}->{'rate'} = $service->total_charges;
            $shipping->{$service->label}->{'rate'} = $ship_amount;
            $shipping->{$service->label}->{'days'} = $service->guaranteed_days;
            
            } else {

            $shipping->{$service->label}->{'rate'} = $service->total_charges;
            $shipping->{$service->label}->{'days'} = $service->guaranteed_days;

            }
            

        } else {

            $shipping->{$service->label}->{'rate'} 
                = $service->total_charges + 3.50;
            $shipping->{$service->label}->{'days'} = $service->guaranteed_days;

        }
    }

    $c->stash->{shipping} = $shipping;

    # Set our template and form to use
    $c->stash( 
        cart_items  => $c->model('Cart')->get_items_in_cart,
        customer    => $customer,
        template    => 'checkout/shipping.tt2',
        form        => $self->shipping_form,
    );
 
    # Process our form
    $form =  $self->shipping_form->process (
        item            => $cart,
        params          => $c->req->params,
    );

    # If the form is processed then we move to payment information. 
    # Else return the form with errors.
    if ( $form ) { 
    # The form was processed

        # Redirect to payment information
        $c->res->redirect( $c->uri_for($self->action_for('payment')) );

    } else { 
    # Username or password were not defined 

        # TODO: display error message via ->flash->{error_msg} ?
        # Mean while we will display a message in debug
        $c->log->debug("not submited");

        return;

    }

}

sub payment : Local {
    my ( $self, $c ) = @_;
    my $auth;
    my $customer;
    my $cart;
    my $form;
    my $total_amount;

    if ($c->user_exists) {
    # Customer is logged in

        # Get our customer object row
        $customer = $c->model('Customer')->get_customer($c->user->id)
    }

    # Get the customer cart by his id
    $cart = $c->model('Cart')->get_cart({ customer_id => $customer->id, });

    # Set our template and form to use
    $c->stash(
        customer        => $customer,
        cart_items      => $c->model('Cart')->get_items_in_cart,
        total_amount    => $c->model('Cart')->total_amount,
        template        => 'checkout/payment.tt2',
        form            => $self->payment_form,
    );

    # Process our form
    $form =  $self->payment_form->process (
        item            => $cart,
        params          => $c->req->params,
    );

    # If the form is processed then we move to payment information.
    # Else return the form with errors.
    if ( $form ) {
    # The form was processed

        # Redirect to payment information
        $c->res->redirect( $c->uri_for($self->action_for('confirmation')) );

    } else {
    # Username or password were not defined

        # TODO: display error message via ->flash->{error_msg} ?
        # Mean while we will display a message in debug
        $c->log->debug("not submited");

        return;

    }

}

sub confirmation : Local {
    my ( $self, $c ) = @_;
    my $customer;
    my $cart;

    if (!$c->user_exists) {
        $c->response->redirect($c->uri_for('/customers/login'));
        return 0;
    }

    # Get the customer cart by his id
    $cart = $c->model('Cart')->get_cart({ customer_id => $c->user->id, });

    # Set our template and form to use
    $c->stash(
        customer        => $c->model('Customer')->get_customer($c->user->id),
        cart            => $cart,
        cart_items      => $c->model('Cart')->get_items_in_cart,
        items_amount    => $c->model('Cart')->sum_items_in_cart,
        promotion       => 
          $c->model('PaquetteDB::Promotion')->get_promotion($cart->promo_code),
        template        => 'checkout/confirmation.tt2',
    );

}

sub process_order : Local {
    my ( $self, $c ) = @_;
    my $order;
    my $cart;
    my $base_url;
    my $img_url;
    my $rtn_url;
    my $cnl_url;
    my $processed;

    # Process the order
    $order = $c->model('Checkout')->process_order;

    if ($order) {

        $c->log->debug($order->id);
        $c->log->debug($order->payment_type);

        if ($order->payment_type eq 'PayPal') {

            $base_url = 'http://saborespanol.com';
            $img_url = $base_url.'/static/images/sabor-espanol-logo.gif';
            $rtn_url = $base_url.'/checkout/complete_order/'. $order->id;
            $cnl_url = $base_url.'/checkout/payment';

            my $paypal = Business::PayPal->new;
            my $button = $paypal->button(
                business        => 'info@saborespanol.com',
                item_name       => 'Saborespanol.com - Shopping Cart',
                invoice         => $order->id,
                image_url       => $img_url,
                receiver_email  => $order->paypal_email,
                return          => $rtn_url, 
                cancel_return   => $cnl_url,
                amount          => $order->payment_amount,
                quantity        => 1,
            );

            $c->stash->{button} = $button;
            $c->stash->{template} = 'checkout/paypal.tt2';

        } elsif ($order->payment_type eq 'CC') {

            $processed 
                = $c->model('Order')->process_cc( { order_id => $order->id } );

            if ($processed->{response} eq 1) {

                $c->res->redirect( 
                    $c->uri_for($self->action_for('receipt'), $order->id) 
                );
                
                $c->model('Cart')->destroy_cart;

            } else {
            # Credit Card Processing was not sucessful

                $c->model('Order')->destroy_order($order->id);

                $c->flash->{payment_failed} 
                   = $processed->{responsetext};
                $c->res->redirect(
                    $c->uri_for($self->action_for('payment'))
                );

            }

        }

    }

        #$c->res->redirect( $c->uri_for($self->action_for('receipt'), $order) );

}

sub complete_order : Local {
    my ( $self, $c, $order_id ) = @_;

    # SECURITY
    # If we we have an order id, then are going to update the order table with
    # the transaction id and status.
    if ($order_id) {
        $c->model('PaquetteDB::Orders')->update_order( {
            order_id    => $order_id,
            txn_status  => $c->req->params->{payment_status},
            txn_id      => $c->req->params->{txn_id},
        } );
    }

    $c->res->redirect( $c->uri_for($self->action_for('receipt'), $order_id) );

}

sub receipt :Chained('') :PathPart('checkout/receipt') : Args(1) {
    my ( $self, $c, $order_id ) = @_;
    my $order;

    $order = $c->model('Order')->get_order({ order_id => $order_id });

    # Load items from cart
    $c->stash(
        order           => $order,
        order_items     => $c->model('Order')->get_items_in_order($order_id),
        items_amount    => $c->model('Order')->sum_items_in_order($order_id),
    );

    $c->stash->{template} = 'checkout/receipt.tt2';

    $c->detach( 'send_email', [$order_id] );

}

sub send_email : Local {
    my ( $self, $c, $order_id) = @_;
    my $subject;
    my $order;

    $order = $c->model('Order')->get_order({ order_id => $order_id });

    # Load items from cart
    $c->stash(
        order           => $order,
        order_items     => $c->model('Order')->get_items_in_order($order_id),
        items_amount    => $c->model('Order')->sum_items_in_order($order_id),
    );

    $subject = 'Order Confirmation from SaborEspanol.com - Order # '. $order_id;

    # Send email
    $c->stash->{email} = {
            to          => $c->stash->{order}->email,
            bcc         => 'sales@saborespanol.com',
            from        => 'sales@saborespanol.com',
            subject     => $subject,
            template    => 'confirm_order.tt2',
            #content_type => 'multipart/alternative'
            content_type => 'text/plain',
        };
        
        $c->forward( $c->view('Email::Template') );

        $c->flash->{status_msg} = "Order # ". $order_id ." submited";

}

sub print_receipt :Chained('') :PathPart('checkout/print_receipt') : Args(1) {
    my ( $self, $c, $order_id ) = @_;
    my $order;

    $order = $c->model('Order')->get_order({ order_id => $order_id });

    # Load items from cart
    $c->stash(
        order           => $order,
        order_items     => $c->model('Order')->get_items_in_order($order_id),
        items_amount    => $c->model('Order')->sum_items_in_order($order_id),
    );

    $c->stash->{template} = 'checkout/print_receipt.tt2';
    $c->forward( $c->view('TTNoWrapper') );


}



=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
