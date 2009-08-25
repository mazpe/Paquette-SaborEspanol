package MyCart;

use Moose;
use MooseX::Types::Moose qw(Str Int ArrayRef);
use namespace::autoclean;
use Data::Dumper;

has 'user'          => ( is => 'ro', required => 1, weak_ref => 1 );
has 'session'       => ( is => 'ro', required => 1, weak_ref => 1 );
has 'session_id'    => ( is => 'ro', required => 1, weak_ref => 1 );
has 'schema'    => ( is => 'rw', required => 1, handles => [qw / resultset /] );
has 'sku'       => ( is => 'ro', isa => Str );
has 'qty'       => ( is => 'ro', isa => Int, default => 0 );

has '_items'    => ( is => 'ro', isa => ArrayRef, lazy_build => 1 );

sub create_cart {
    my ( $self, $customer_id ) = @_;
    my $cart;
    my $cart_args;

    # Create a hashref with our cart arguments 
    $cart_args = ( { 
        session_id  => $self->session_id,
        customer_id => $customer_id,
    } );

    # Create the Cart
    $cart = $self->resultset('Cart')->create( $cart_args );
    
    return $cart;
}

sub get_cart {
    my $self            = shift;
    my $cart_id         = get_cart_id($self);
    my $cart;

print "here ". $cart_id;

    # Check for a cart with the same session id
    if ( $cart_id ) {
    # Found cart with the same Session ID

        # Get our cart id from database
        $cart = $self->resultset('Cart')->get_cart($cart_id);

    } 

    return $cart;
}


sub get_cart_id {
    my $self            = shift;
    my $session_id      = $self->session_id;
    my $cart_args;
    my $cart_id;


    if ( $self->session->{cart_id} gt 0 ) {
    # There is a cart_id in the session

        # Get our Cart ID from session
        $cart_id = $self->session->{cart_id};

        # Set arguments for validation
        $cart_args = {
            cart_id     => $cart_id,
            session_id  => $session_id,
        };

        # Check if Cart ID still valid, if not create a new cart
        unless ( $self->resultset('Cart')->validate_cart_id($cart_args) == 1 ) {
        # Cart_id from session not valid

            $cart_id = create_cart($self)->id;
        }

    } else {
    # Card ID not found

        # Create Cart ID
        $cart_id = create_cart($self)->id;

    }
        
    $self->session->{cart_id} = $cart_id;

    return $cart_id;
}

# Adds items to the shopping cart
sub add_items_to_cart {
    my ( $self, $args ) = @_;
    my $cart_id         = get_cart_id($self);
    my $item;

    # Check that we do have this item
    if ( $self->resultset('Product')->get_item_by_sku($args->{sku}) ) {
    # Sku found on db

        $item = {
            cart_id     => $cart_id,
            cart_sku    => $cart_id.''.$args->{sku},
            sku         => $args->{sku},
            quantity    => $args->{qty},
        };

        # Add item to cart
        $self->resultset('CartItem')->add_item( $item );
        
    } else {
    # Sku not found

    }

}

sub sum_items_in_cart {
    my $self = shift;
    my $cart_id         = get_cart_id($self);
    my $total_amount;

    $total_amount
        = $self->resultset('CartItem')->sum_items($cart_id);

    return $total_amount;
}

# Return list of items
sub get_items_in_cart { 
    my $self            = shift;
    my $cart_id         = get_cart_id($self);

    my $items_in_cart 
            = $self->resultset('CartItem')->get_items( $cart_id );

    return $items_in_cart ? $items_in_cart : 0;
}

# Removes items from the shopping cart
sub remove_items_from_cart { 
    my ( $self, $args ) = @_;
    my $cart_id         = get_cart_id($self);
    my $cart_sku        = $cart_id.''.$args->{sku};

    # Remove particular item from cart
    $self->resultset('CartItem')->remove_item($cart_sku);

    return 1;
}

# Update items in the shopping cart
sub update_items_in_cart {
    my ( $self, $args ) = @_;
    my $cart_id         = get_cart_id($self);
    my $items           = $args->{items};

    # Loop through each sku
    foreach my $sku ( keys %$items ) {

        # Create our cart_sku identifyer
        my $cart_sku = $cart_id.''.$sku;

        # Update item in the cart
        my $item = $self->resultset('CartItem')->update_item( {
            quantity    => $items->{$sku},
            cart_sku    => $cart_sku,
        } );

    }

    return 1;
}

# Clear items in the shopping cart
sub clear_cart {
    my $self            = shift; 
    my $cart_id         = get_cart_id($self);

    # Clear our shopping cart items
    $self->resultset('CartItem')->clear_items($cart_id);

    return 1;
}

sub assign_cart {
    my $self = shift;
    my $customer_id;
    my $customer_cart_id;
    my $anon_cart_id;
    my %cart_args;

    if ( $self->user ) {
    # Logged in

        # Get our customer id
        $customer_id
            = $self->resultset('Customer')->get_customer_by_email(
                $self->user->username
              )->{id};

        # Do we own a cart? whats our cart_id
        $customer_cart_id 
            = $self->resultset('Cart')->get_cart_by_cid($customer_id)->{id};

        # Do we have an anonymous cart? what our anonymous cart id
        $anon_cart_id
            = $self->resultset('Cart')->get_cart_by_sid(
                $self->session_id
              )->{id};

        # Check that we have 2 seperate carts
        if ( $customer_cart_id ) {
        # Found customer cart

            if ( $customer_cart_id ne $anon_cart_id ) {
            # Seperate carts

                # Update Cart Items to the customer_cart
                $self->resultset('CartItem')->set_items_cart_sku(
                    $customer_cart_id,
                    $anon_cart_id,
                );

                # Delete anonymous cart
                $self->resultset('Cart')->delete($anon_cart_id);

                # Update our customer cart with our current session
                %cart_args = (
                    id          => $customer_cart_id,
                    session_id  => $self->session_id,
                );
                $self->resultset('Cart')->update(\%cart_args);

            }

        } else {
        # No customer
            # Update our customer cart with our current session
            %cart_args = (
                id              => $anon_cart_id,
                session_id      => $self->session_id,
                customer_id     => $customer_id,
            );
            $self->resultset('Cart')->update(\%cart_args);

        }
        
        
    } else {
    # Not logged in


    }

}

sub assign_cart2 {
    my $self            = shift;
    my $cart_id;
    my $customer_id;
    my $new_cart_id;
    my $cart;
    my $old_cart_id;
    my %cart_args;

    if ( $self->user ) {
    # User is logged in

        # Get our customer id
        $customer_id 
            = $self->resultset('Customer')->get_customer_by_email(
                $self->user->username
              )->{id};
        #$customer_id = $customer_id->{id};

        # Check if user already has a cart
        # Get a copy of the cart
        $cart = $self->resultset('Cart')->get_cart_by_cid($customer_id);

        # Found cart and retrived ResultSet
        # Update our new cart with found cart
        if ( $cart ) {
        # We found a cart 
            
            $old_cart_id = $cart->{id};
            
            %cart_args = (
                session_id  => $self->session_id,
                customer_id => $customer_id,
                id          => $cart_id,
            );        

            # Set our new cart info
            $new_cart_id = 
                $self->resultset('Cart')->set_cart_info( $cart_id, $cart );

            # Update found cart_items with our new cart
            $self->resultset('CartItem')->set_items_cart_id ( 
                $cart_id, 
                $old_cart_id,
            );

            # Setup items cart_sku
            $self->resultset('CartItem')->set_items_cart_sku(
                $cart_id,
                $old_cart_id,
            );

            if ( $new_cart_id ne $old_cart_id ) {
                # Delete old cart;
                $self->resultset('Cart')->delete($old_cart_id);
            }

        } else {
        # No cart found

        }

    } else {
    # Anonymous user 

        # Attached Cart and Customer 
        $self->resultset('Cart')->attach_cart_to_customer( { 
            customer_id => $self->user->id,
            cart_id     => $cart_id,
        } );
    }

}

sub set_shipping {
    my ( $self, $args ) = @_;
    my $cart_id         = get_cart_id($self);

    $args->{id} = $cart_id;
    delete $args->{submit};
    
    # Set shipping
    $self->resultset('Cart')->set_shipping_info($args);
}

sub set_payment {
    my ( $self, $args ) = @_;
    my $cart_id         = get_cart_id($self);

    $args->{id} = $cart_id;
    delete $args->{submit};

    # Set payment
    $self->resultset('Cart')->set_payment_info($args);
}

sub total_items_in_cart {
    my $self            = shift;
    my $cart_id         = get_cart_id($self);


    my $total_items = $self->resultset('CartItem')->count_items($cart_id);

    return $total_items;
}

1;
